#include "thresh/forward_pass.hpp"
#include "thresh/render_graph.hpp"
#include "flux/render_graph_pass.hpp"
#include "flux/render_graph_resource.hpp"
#include "flux/vertex.hpp"
#include "substratum/log.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <cstring>
#include <stdexcept>

namespace thresh {

    static constexpr std::uint32_t INITIAL_OBJECT_CAPACITY = 256;
    static constexpr std::uint32_t INITIAL_POINT_LIGHT_CAPACITY = 32;

    ForwardPass::ForwardPass(const Config &config)
        : m_device{config.device}
        , m_material_system{config.material_system} {

        // --- Shader program ---
        auto push_range = VkPushConstantRange{};
        push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        push_range.offset = 0;
        push_range.size = sizeof(std::uint32_t); // object_index

        // Create descriptor resources first (need layouts for shader program)
        create_descriptor_resources();
        create_texture_descriptor_resources();

        // Create shared sampler for texture sampling
        m_sampler = std::make_unique<flux::Sampler>(flux::Sampler::Config{
            .device = m_device->get_logical_device()
        });

        m_shader_program = std::make_unique<flux::ShaderProgram>(flux::ShaderProgram::Config{
            .device = m_device,
            .name = "forward_pbr",
            .stages = {
                {
                    .filepath = config.shader_dir / "forward.vert",
                    .stage = flux::ShaderObject::Stage::Vertex,
                    .next_stage = VK_SHADER_STAGE_FRAGMENT_BIT
                },
                {
                    .filepath = config.shader_dir / "forward.frag",
                    .stage = flux::ShaderObject::Stage::Fragment,
                    .next_stage = static_cast<VkShaderStageFlagBits>(0)
                }
            },
            .set_layouts = {m_set0_layout, m_set1_layout},
            .push_constant_ranges = {push_range},
            .mode = flux::ShaderProgram::Mode::Unlinked,
            .enable_hot_reload = true
        });

        // --- Graphics state (standard mesh defaults + our vertex format) ---
        m_graphics_state = flux::GraphicsState::create_mesh_default();
        m_graphics_state.vertex_bindings = flux::get_vertex_bindings();
        m_graphics_state.vertex_attributes = flux::get_vertex_attributes();

        // Counter-clockwise front face (glTF convention)
        m_graphics_state.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        // Disable backface culling (many glTF models use double-sided materials)
        m_graphics_state.cull_mode = VK_CULL_MODE_NONE;

        // --- GPU buffers ---
        create_ubo_buffers();
        create_object_ssbo_buffers();
        create_point_light_ssbo_buffers();

        SUB_INFO("ForwardPass initialized");
    }

    ForwardPass::~ForwardPass() {
        auto vk_device = m_device->get_logical_device();

        // Destroy UBO buffers
        for (auto &ubo : m_ubo_buffers) {
            if (ubo.mapped) {
                ::vkUnmapMemory(vk_device, ubo.memory);
                ubo.mapped = nullptr;
            }
            destroy_buffer(ubo.buffer, ubo.memory);
        }

        // Destroy object SSBO buffers
        for (auto &ssbo : m_object_ssbo_buffers) {
            if (ssbo.mapped) {
                ::vkUnmapMemory(vk_device, ssbo.memory);
            }
            destroy_buffer(ssbo.buffer, ssbo.memory);
        }

        // Destroy point light SSBO buffers
        for (auto &ssbo : m_point_light_ssbo_buffers) {
            if (ssbo.mapped) {
                ::vkUnmapMemory(vk_device, ssbo.memory);
            }
            destroy_buffer(ssbo.buffer, ssbo.memory);
        }

        // Destroy sampler
        m_sampler.reset();

        // Destroy descriptors
        if (m_texture_pool != VK_NULL_HANDLE) {
            ::vkDestroyDescriptorPool(vk_device, m_texture_pool, nullptr);
        }
        if (m_set1_layout != VK_NULL_HANDLE) {
            ::vkDestroyDescriptorSetLayout(vk_device, m_set1_layout, nullptr);
        }
        if (m_descriptor_pool != VK_NULL_HANDLE) {
            ::vkDestroyDescriptorPool(vk_device, m_descriptor_pool, nullptr);
        }
        if (m_set0_layout != VK_NULL_HANDLE) {
            ::vkDestroyDescriptorSetLayout(vk_device, m_set0_layout, nullptr);
        }
    }

    // ── Graph setup ─────────────────────────────────────────────────────────────

    auto ForwardPass::setup_graph(
        RenderGraph &graph,
        VkExtent2D swapchain_extent,
        FrameDataProvider frame_data_provider
    ) -> void {
        auto &builder = graph.begin_build();
        auto backbuffer = graph.get_backbuffer_handle();

        // Create transient depth image
        auto depth_image = builder.create_image(
            "depth",
            flux::ImageResourceDesc::create_2d(
                VK_FORMAT_D32_SFLOAT,
                swapchain_extent.width,
                swapchain_extent.height,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            )
        );

        // Capture pointers for lambda
        auto *self = this;
        auto provider = std::move(frame_data_provider);

        builder.add_graphics_pass("forward_pass",
                [self, provider](const flux::PassExecutionContext &ctx) {
                    auto cmd = ctx.command_buffer;
                    auto frame_index = ctx.frame_index;

                    // Fetch current frame data and upload to GPU
                    auto frame_data = provider();
                    if (frame_data.render_data && frame_data.meshes) {
                        self->update_frame_data(frame_index, *frame_data.render_data, *frame_data.meshes);
                    }

                    // Set viewport/scissor from extent
                    auto state = self->m_graphics_state;
                    state.viewports = {{
                        .x = 0.0f,
                        .y = 0.0f,
                        .width = static_cast<float>(ctx.render_extent.width),
                        .height = static_cast<float>(ctx.render_extent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f
                    }};
                    state.scissors = {{
                        .offset = {0, 0},
                        .extent = ctx.render_extent
                    }};

                    state.apply(cmd, *ctx.device);
                    self->m_shader_program->bind(cmd);

                    // Bind descriptor sets 0 and 1
                    VkDescriptorSet sets[] = {
                        self->m_descriptor_sets[frame_index],
                        self->m_texture_descriptor_set
                    };
                    ::vkCmdBindDescriptorSets(
                        cmd,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        self->m_shader_program->get_pipeline_layout(),
                        0, 2, sets,
                        0, nullptr
                    );

                    const auto &cache = self->m_frame_caches[frame_index];
                    if (!self->m_meshes_ptr || cache.object_count == 0) return;

                    const auto &meshes = *self->m_meshes_ptr;

                    // Draw each object
                    for (std::uint32_t i = 0; i < cache.object_count; ++i) {
                        const auto &obj = cache.objects[i];

                        if (obj.mesh_index >= meshes.size() || !meshes[obj.mesh_index]) continue;

                        const auto &mesh = *meshes[obj.mesh_index];

                        // Push object index
                        ::vkCmdPushConstants(
                            cmd,
                            self->m_shader_program->get_pipeline_layout(),
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                            0, sizeof(std::uint32_t), &i
                        );

                        // Bind mesh and draw all submeshes
                        mesh.bind(cmd);
                        mesh.draw_all(cmd);
                    }
                })
            .set_color_attachment(0, backbuffer, VK_ATTACHMENT_LOAD_OP_CLEAR,
                // THRESH Deep: #06080C
                {.float32 = {0.024f, 0.031f, 0.047f, 1.0f}})
            .set_depth_attachment(depth_image, VK_ATTACHMENT_LOAD_OP_CLEAR, {1.0f, 0});
    }

    // ── Frame data upload ───────────────────────────────────────────────────────

    auto ForwardPass::update_frame_data(
        std::uint32_t frame_index,
        const FrameRenderData &render_data,
        const std::vector<std::unique_ptr<flux::Mesh>> &meshes
    ) -> void {
        m_meshes_ptr = &meshes;

        // --- Upload UBO ---
        auto light_count = static_cast<std::uint32_t>(render_data.point_lights.size());

        FrameUBO ubo{};
        ubo.view = render_data.view;
        ubo.proj = render_data.projection;
        ubo.camera_pos = render_data.camera_position;
        ubo.sun_direction = render_data.sun.direction;
        ubo.sun_intensity = render_data.sun.intensity;
        ubo.sun_color = render_data.sun.color;
        ubo.ambient_color = render_data.ambient.color;
        ubo.ambient_intensity = render_data.ambient.intensity;
        ubo.point_light_count = light_count;

        std::memcpy(m_ubo_buffers[frame_index].mapped, &ubo, sizeof(FrameUBO));

        // --- Upload object SSBO ---
        auto object_count = static_cast<std::uint32_t>(render_data.objects.size());
        grow_object_ssbo_if_needed(object_count);

        if (object_count > 0) {
            auto *dst = static_cast<GpuObjectData *>(m_object_ssbo_buffers[frame_index].mapped);
            for (std::uint32_t i = 0; i < object_count; ++i) {
                const auto &obj = render_data.objects[i];
                dst[i].model = obj.model_matrix;
                dst[i].normal_matrix = glm::inverseTranspose(obj.model_matrix);
                dst[i].material_index = obj.material_index;
                dst[i]._pad0 = 0;
                dst[i]._pad1 = 0;
                dst[i]._pad2 = 0;
            }
        }

        // --- Upload point light SSBO ---
        grow_point_light_ssbo_if_needed(light_count);

        if (light_count > 0) {
            auto *dst = static_cast<GpuPointLight *>(m_point_light_ssbo_buffers[frame_index].mapped);
            for (std::uint32_t i = 0; i < light_count; ++i) {
                const auto &pl = render_data.point_lights[i];
                dst[i].position = pl.position;
                dst[i].radius = pl.radius;
                dst[i].color = pl.color;
                dst[i].intensity = pl.intensity;
            }
        }

        // --- Update material SSBO ---
        m_material_system->update_gpu_data(frame_index);

        // --- Update texture descriptors (only when new textures are added) ---
        update_texture_descriptors();

        // --- Update descriptors (in case buffers were reallocated) ---
        update_descriptors(frame_index);

        // --- Cache render objects for execute callback ---
        auto &cache = m_frame_caches[frame_index];
        cache.object_count = object_count;
        cache.objects = render_data.objects;
    }

    // ── Descriptor setup ────────────────────────────────────────────────────────

    auto ForwardPass::create_descriptor_resources() -> void {
        auto vk_device = m_device->get_logical_device();

        // --- Descriptor set layout ---
        std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

        // Binding 0: Frame UBO
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 1: Object SSBO
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        // Binding 2: Material SSBO
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 3: Point Light SSBO
        bindings[3].binding = 3;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        if (::vkCreateDescriptorSetLayout(vk_device, &layout_info, nullptr, &m_set0_layout) != VK_SUCCESS) {
            SUB_FATAL("Failed to create ForwardPass descriptor set layout");
            throw std::runtime_error("Failed to create descriptor set layout");
        }

        // --- Descriptor pool ---
        std::array<VkDescriptorPoolSize, 2> pool_sizes{};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_sizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT * 3; // Object + Material + PointLight SSBOs

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = MAX_FRAMES_IN_FLIGHT;
        pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        if (::vkCreateDescriptorPool(vk_device, &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS) {
            SUB_FATAL("Failed to create ForwardPass descriptor pool");
            throw std::runtime_error("Failed to create descriptor pool");
        }

        // --- Allocate descriptor sets ---
        std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{};
        layouts.fill(m_set0_layout);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        alloc_info.pSetLayouts = layouts.data();

        if (::vkAllocateDescriptorSets(vk_device, &alloc_info, m_descriptor_sets.data()) != VK_SUCCESS) {
            SUB_FATAL("Failed to allocate ForwardPass descriptor sets");
            throw std::runtime_error("Failed to allocate descriptor sets");
        }
    }

    auto ForwardPass::create_texture_descriptor_resources() -> void {
        auto vk_device = m_device->get_logical_device();

        // --- Set 1 layout: sampler + bindless texture array ---
        // VARIABLE_DESCRIPTOR_COUNT must be on the highest binding number,
        // so sampler = binding 0, textures = binding 1.
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

        // Binding 0: shared sampler
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 1: sampled image array (variable count, partially bound)
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[1].descriptorCount = MAX_TEXTURE_COUNT;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding flags: variable-count partially-bound on the texture array (binding 1)
        std::array<VkDescriptorBindingFlags, 2> binding_flags{};
        binding_flags[0] = 0;
        binding_flags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                         | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info{};
        flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flags_info.bindingCount = static_cast<std::uint32_t>(binding_flags.size());
        flags_info.pBindingFlags = binding_flags.data();

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pNext = &flags_info;
        layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        if (::vkCreateDescriptorSetLayout(vk_device, &layout_info, nullptr, &m_set1_layout) != VK_SUCCESS) {
            SUB_FATAL("Failed to create texture descriptor set layout");
            throw std::runtime_error("Failed to create texture descriptor set layout");
        }

        // --- Texture descriptor pool ---
        std::array<VkDescriptorPoolSize, 2> pool_sizes{};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        pool_sizes[0].descriptorCount = MAX_TEXTURE_COUNT;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        pool_sizes[1].descriptorCount = 1;

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        if (::vkCreateDescriptorPool(vk_device, &pool_info, nullptr, &m_texture_pool) != VK_SUCCESS) {
            SUB_FATAL("Failed to create texture descriptor pool");
            throw std::runtime_error("Failed to create texture descriptor pool");
        }

        // --- Allocate texture descriptor set with variable count ---
        std::uint32_t variable_count = MAX_TEXTURE_COUNT;
        VkDescriptorSetVariableDescriptorCountAllocateInfo variable_info{};
        variable_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
        variable_info.descriptorSetCount = 1;
        variable_info.pDescriptorCounts = &variable_count;

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = &variable_info;
        alloc_info.descriptorPool = m_texture_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_set1_layout;

        if (::vkAllocateDescriptorSets(vk_device, &alloc_info, &m_texture_descriptor_set) != VK_SUCCESS) {
            SUB_FATAL("Failed to allocate texture descriptor set");
            throw std::runtime_error("Failed to allocate texture descriptor set");
        }
    }

    auto ForwardPass::update_texture_descriptors() -> void {
        auto tex_count = m_material_system->get_texture_count();
        if (tex_count == m_last_texture_count) return;

        m_last_texture_count = tex_count;
        if (tex_count == 0) return;

        const auto &textures = m_material_system->get_textures();

        // Write image descriptors for all textures
        std::vector<VkDescriptorImageInfo> image_infos(tex_count);
        for (std::uint32_t i = 0; i < tex_count; ++i) {
            image_infos[i].sampler = VK_NULL_HANDLE;
            image_infos[i].imageView = textures[i]->get_image().get_view();
            image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        // Write sampler descriptor
        VkDescriptorImageInfo sampler_info{};
        sampler_info.sampler = m_sampler->get_handle();

        std::array<VkWriteDescriptorSet, 2> writes{};

        // Binding 0: sampler
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_texture_descriptor_set;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &sampler_info;

        // Binding 1: texture array
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_texture_descriptor_set;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].descriptorCount = tex_count;
        writes[1].pImageInfo = image_infos.data();

        ::vkUpdateDescriptorSets(m_device->get_logical_device(),
                                  static_cast<std::uint32_t>(writes.size()),
                                  writes.data(), 0, nullptr);

        SUB_DEBUG("Updated texture descriptors: {} textures bound", tex_count);
    }

    auto ForwardPass::update_descriptors(std::uint32_t frame_index) -> void {
        std::array<VkWriteDescriptorSet, 4> writes{};
        std::array<VkDescriptorBufferInfo, 4> buffer_infos{};

        // UBO
        buffer_infos[0].buffer = m_ubo_buffers[frame_index].buffer;
        buffer_infos[0].offset = 0;
        buffer_infos[0].range = sizeof(FrameUBO);

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_descriptor_sets[frame_index];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &buffer_infos[0];

        // Object SSBO
        auto &obj_ssbo = m_object_ssbo_buffers[frame_index];
        VkDeviceSize obj_range = std::max(
            static_cast<VkDeviceSize>(obj_ssbo.capacity) * sizeof(GpuObjectData),
            static_cast<VkDeviceSize>(sizeof(GpuObjectData))
        );
        buffer_infos[1].buffer = obj_ssbo.buffer;
        buffer_infos[1].offset = 0;
        buffer_infos[1].range = obj_range;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_descriptor_sets[frame_index];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo = &buffer_infos[1];

        // Material SSBO
        auto mat_ssbo_size = m_material_system->get_material_ssbo_size();
        if (mat_ssbo_size == 0) mat_ssbo_size = sizeof(GpuMaterialData);
        buffer_infos[2].buffer = m_material_system->get_material_ssbo(frame_index);
        buffer_infos[2].offset = 0;
        buffer_infos[2].range = mat_ssbo_size;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = m_descriptor_sets[frame_index];
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &buffer_infos[2];

        // Point Light SSBO
        auto &pl_ssbo = m_point_light_ssbo_buffers[frame_index];
        VkDeviceSize pl_range = std::max(
            static_cast<VkDeviceSize>(pl_ssbo.capacity) * sizeof(GpuPointLight),
            static_cast<VkDeviceSize>(sizeof(GpuPointLight))
        );
        buffer_infos[3].buffer = pl_ssbo.buffer;
        buffer_infos[3].offset = 0;
        buffer_infos[3].range = pl_range;

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].dstSet = m_descriptor_sets[frame_index];
        writes[3].dstBinding = 3;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[3].descriptorCount = 1;
        writes[3].pBufferInfo = &buffer_infos[3];

        ::vkUpdateDescriptorSets(m_device->get_logical_device(),
                                  static_cast<std::uint32_t>(writes.size()),
                                  writes.data(), 0, nullptr);
    }

    // ── Buffer management ───────────────────────────────────────────────────────

    auto ForwardPass::create_ubo_buffers() -> void {
        for (auto &ubo : m_ubo_buffers) {
            m_device->create_buffer(
                sizeof(FrameUBO),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                ubo.buffer, ubo.memory
            );
            ::vkMapMemory(m_device->get_logical_device(), ubo.memory, 0, sizeof(FrameUBO), 0, &ubo.mapped);
        }
    }

    auto ForwardPass::create_object_ssbo_buffers() -> void {
        VkDeviceSize size = INITIAL_OBJECT_CAPACITY * sizeof(GpuObjectData);
        for (auto &ssbo : m_object_ssbo_buffers) {
            m_device->create_buffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                ssbo.buffer, ssbo.memory
            );
            ::vkMapMemory(m_device->get_logical_device(), ssbo.memory, 0, size, 0, &ssbo.mapped);
            ssbo.capacity = INITIAL_OBJECT_CAPACITY;
        }
    }

    auto ForwardPass::grow_object_ssbo_if_needed(std::uint32_t object_count) -> void {
        if (object_count == 0) return;

        bool needs_grow = false;
        for (const auto &ssbo : m_object_ssbo_buffers) {
            if (ssbo.capacity < object_count) {
                needs_grow = true;
                break;
            }
        }

        if (!needs_grow) return;

        std::uint32_t new_capacity = m_object_ssbo_buffers[0].capacity;
        while (new_capacity < object_count) {
            new_capacity *= 2;
        }

        SUB_DEBUG("Growing object SSBO: {} -> {} objects", m_object_ssbo_buffers[0].capacity, new_capacity);

        m_device->wait_idle();

        VkDeviceSize new_size = static_cast<VkDeviceSize>(new_capacity) * sizeof(GpuObjectData);

        for (auto &ssbo : m_object_ssbo_buffers) {
            if (ssbo.mapped) {
                ::vkUnmapMemory(m_device->get_logical_device(), ssbo.memory);
                ssbo.mapped = nullptr;
            }
            destroy_buffer(ssbo.buffer, ssbo.memory);

            m_device->create_buffer(
                new_size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                ssbo.buffer, ssbo.memory
            );
            ::vkMapMemory(m_device->get_logical_device(), ssbo.memory, 0, new_size, 0, &ssbo.mapped);
            ssbo.capacity = new_capacity;
        }
    }

    auto ForwardPass::create_point_light_ssbo_buffers() -> void {
        VkDeviceSize size = INITIAL_POINT_LIGHT_CAPACITY * sizeof(GpuPointLight);
        for (auto &ssbo : m_point_light_ssbo_buffers) {
            m_device->create_buffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                ssbo.buffer, ssbo.memory
            );
            ::vkMapMemory(m_device->get_logical_device(), ssbo.memory, 0, size, 0, &ssbo.mapped);
            ssbo.capacity = INITIAL_POINT_LIGHT_CAPACITY;
        }
    }

    auto ForwardPass::grow_point_light_ssbo_if_needed(std::uint32_t light_count) -> void {
        if (light_count == 0) return;

        bool needs_grow = false;
        for (const auto &ssbo : m_point_light_ssbo_buffers) {
            if (ssbo.capacity < light_count) {
                needs_grow = true;
                break;
            }
        }

        if (!needs_grow) return;

        std::uint32_t new_capacity = m_point_light_ssbo_buffers[0].capacity;
        while (new_capacity < light_count) {
            new_capacity *= 2;
        }

        SUB_DEBUG("Growing point light SSBO: {} -> {} lights", m_point_light_ssbo_buffers[0].capacity, new_capacity);

        m_device->wait_idle();

        VkDeviceSize new_size = static_cast<VkDeviceSize>(new_capacity) * sizeof(GpuPointLight);

        for (auto &ssbo : m_point_light_ssbo_buffers) {
            if (ssbo.mapped) {
                ::vkUnmapMemory(m_device->get_logical_device(), ssbo.memory);
                ssbo.mapped = nullptr;
            }
            destroy_buffer(ssbo.buffer, ssbo.memory);

            m_device->create_buffer(
                new_size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                ssbo.buffer, ssbo.memory
            );
            ::vkMapMemory(m_device->get_logical_device(), ssbo.memory, 0, new_size, 0, &ssbo.mapped);
            ssbo.capacity = new_capacity;
        }
    }

    auto ForwardPass::destroy_buffer(VkBuffer &buffer, VkDeviceMemory &memory) -> void {
        auto vk_device = m_device->get_logical_device();
        if (buffer != VK_NULL_HANDLE) {
            ::vkDestroyBuffer(vk_device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            ::vkFreeMemory(vk_device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

} // namespace thresh
