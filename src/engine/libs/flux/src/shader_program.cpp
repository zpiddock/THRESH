#include "flux/shader_program.hpp"
#include "flux/device.hpp"
#include "flux/shader_compiler.hpp"
#include "substratum/log.hpp"
#include <stdexcept>
#include <fstream>

namespace flux {

    ShaderProgram::ShaderProgram(const Config &config)
        : m_device{config.device}
          , m_name{config.name}
          , m_mode{config.mode}
          , m_set_layouts{config.set_layouts}
          , m_push_constant_ranges{config.push_constant_ranges}
          , m_stage_configs{config.stages}
          , m_hot_reload_enabled{config.enable_hot_reload} {
        SUB_INFO("Creating ShaderProgram '{}' ({} stages, {} mode)",
                 m_name,
                 config.stages.size(),
                 m_mode == Mode::Linked ? "linked" : "unlinked");

        create_pipeline_layout();

        if (m_mode == Mode::Unlinked) {
            // Create individual ShaderObjects
            for (const auto &stage_config: m_stage_configs) {
                ShaderObject::Config so_config{};
                so_config.device = m_device;
                so_config.filepath = stage_config.filepath;
                so_config.stage = stage_config.stage;
                so_config.next_stage = stage_config.next_stage;
                so_config.set_layouts = m_set_layouts;
                so_config.push_constant_ranges = m_push_constant_ranges;
                so_config.enable_hot_reload = m_hot_reload_enabled;
                so_config.optimize = !m_hot_reload_enabled;
                so_config.throw_on_error = true;

                auto shader = std::make_unique<ShaderObject>(so_config);
                m_stage_flags.push_back(shader->get_stage());
                m_unlinked_shaders.push_back(std::move(shader));
            }
        } else {
            // Create linked shaders in a single batch
            create_linked_shaders();
        }

        SUB_INFO("ShaderProgram '{}' created successfully", m_name);
    }

    ShaderProgram::~ShaderProgram() {
        cleanup();
    }

    ShaderProgram::ShaderProgram(ShaderProgram &&other) noexcept
        : m_device{other.m_device}
          , m_name{std::move(other.m_name)}
          , m_mode{other.m_mode}
          , m_pipeline_layout{other.m_pipeline_layout}
          , m_set_layouts{std::move(other.m_set_layouts)}
          , m_push_constant_ranges{std::move(other.m_push_constant_ranges)}
          , m_stage_configs{std::move(other.m_stage_configs)}
          , m_unlinked_shaders{std::move(other.m_unlinked_shaders)}
          , m_linked_shaders{std::move(other.m_linked_shaders)}
          , m_linked_stages{std::move(other.m_linked_stages)}
          , m_stage_flags{std::move(other.m_stage_flags)}
          , m_hot_reload_enabled{other.m_hot_reload_enabled} {
        other.m_device = nullptr;
        other.m_pipeline_layout = VK_NULL_HANDLE;
    }

    auto ShaderProgram::operator=(ShaderProgram &&other) noexcept -> ShaderProgram & {
        if (this != &other) {
            cleanup();

            m_device = other.m_device;
            m_name = std::move(other.m_name);
            m_mode = other.m_mode;
            m_pipeline_layout = other.m_pipeline_layout;
            m_set_layouts = std::move(other.m_set_layouts);
            m_push_constant_ranges = std::move(other.m_push_constant_ranges);
            m_stage_configs = std::move(other.m_stage_configs);
            m_unlinked_shaders = std::move(other.m_unlinked_shaders);
            m_linked_shaders = std::move(other.m_linked_shaders);
            m_linked_stages = std::move(other.m_linked_stages);
            m_stage_flags = std::move(other.m_stage_flags);
            m_hot_reload_enabled = other.m_hot_reload_enabled;

            other.m_device = nullptr;
            other.m_pipeline_layout = VK_NULL_HANDLE;
        }
        return *this;
    }

    auto ShaderProgram::bind(VkCommandBuffer cmd) const -> void {
        const auto &fn = m_device->get_shader_object_fn();

        if (m_mode == Mode::Unlinked) {
            // Collect handles from individual ShaderObjects
            std::vector<VkShaderEXT> handles;
            handles.reserve(m_unlinked_shaders.size());
            for (const auto &shader: m_unlinked_shaders) {
                handles.push_back(shader->get_handle());
            }

            fn.cmd_bind_shaders(cmd,
                static_cast<std::uint32_t>(m_stage_flags.size()),
                m_stage_flags.data(),
                handles.data());
        } else {
            // Use pre-built linked shader handles
            fn.cmd_bind_shaders(cmd,
                static_cast<std::uint32_t>(m_linked_stages.size()),
                m_linked_stages.data(),
                m_linked_shaders.data());
        }
    }

    auto ShaderProgram::check_and_reload() -> bool {
        if (m_mode != Mode::Unlinked) {
            return false;
        }

        bool any_reloaded = false;
        for (auto &shader: m_unlinked_shaders) {
            if (shader->check_and_reload()) {
                any_reloaded = true;
            }
        }

        return any_reloaded;
    }

    [[nodiscard]] auto ShaderProgram::is_valid() const -> bool {
        if (m_pipeline_layout == VK_NULL_HANDLE) {
            return false;
        }

        if (m_mode == Mode::Unlinked) {
            for (const auto &shader: m_unlinked_shaders) {
                if (!shader->is_valid()) {
                    return false;
                }
            }
            return !m_unlinked_shaders.empty();
        }

        // Linked mode
        for (const auto &shader: m_linked_shaders) {
            if (shader == VK_NULL_HANDLE) {
                return false;
            }
        }
        return !m_linked_shaders.empty();
    }

    auto ShaderProgram::create_pipeline_layout() -> void {
        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = static_cast<std::uint32_t>(m_set_layouts.size());
        layout_info.pSetLayouts = m_set_layouts.empty() ? nullptr : m_set_layouts.data();
        layout_info.pushConstantRangeCount = static_cast<std::uint32_t>(m_push_constant_ranges.size());
        layout_info.pPushConstantRanges = m_push_constant_ranges.empty()
                                              ? nullptr
                                              : m_push_constant_ranges.data();

        if (::vkCreatePipelineLayout(m_device->get_logical_device(), &layout_info, nullptr, &m_pipeline_layout) !=
            VK_SUCCESS) {
            SUB_FATAL("Failed to create pipeline layout for ShaderProgram '{}'", m_name);
            throw std::runtime_error("Failed to create pipeline layout for ShaderProgram '" + m_name + "'");
        }
    }

    auto ShaderProgram::create_linked_shaders() -> void {
        SUB_INFO("Creating linked shaders for program '{}'", m_name);

        const auto &fn = m_device->get_shader_object_fn();

        // Load SPIR-V for each stage
        std::vector<std::vector<std::uint32_t>> all_spirv;
        all_spirv.reserve(m_stage_configs.size());

        for (const auto &stage_config: m_stage_configs) {
            // Reuse ShaderObject's load logic by constructing a temporary
            // ShaderObject just for loading (or inline the load logic)
            auto extension = stage_config.filepath.extension().string();
            bool is_glsl = (extension == ".vert" || extension == ".frag"
                            || extension == ".comp" || extension == ".geom"
                            || extension == ".tesc" || extension == ".tese"
                            || extension == ".glsl");

            if (is_glsl) {
                ShaderCompiler compiler;
                ShaderCompiler::CompileOptions opts;
                opts.stage = static_cast<ShaderCompiler::Stage>(stage_config.stage);
                opts.optimization = ShaderCompiler::OptimizationLevel::Performance;
                opts.generate_debug_info = false;

                auto result = compiler.compile_file(stage_config.filepath, opts);
                if (!result.success) {
                    throw std::runtime_error("Linked shader compilation failed for '"
                                             + stage_config.filepath.string() + "': " + result.error_message);
                }
                all_spirv.push_back(std::move(result.spirv));
            } else {
                std::ifstream file(stage_config.filepath, std::ios::ate | std::ios::binary);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open shader file: " + stage_config.filepath.string());
                }
                auto file_size = static_cast<std::size_t>(file.tellg());
                std::vector<std::uint32_t> spirv(file_size / sizeof(std::uint32_t));
                file.seekg(0);
                file.read(reinterpret_cast<char *>(spirv.data()), static_cast<std::streamsize>(file_size));
                all_spirv.push_back(std::move(spirv));
            }
        }

        // Build create infos with link flag
        std::vector<VkShaderCreateInfoEXT> create_infos;
        create_infos.reserve(m_stage_configs.size());

        for (std::size_t i = 0; i < m_stage_configs.size(); ++i) {
            VkShaderCreateInfoEXT info{};
            info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
            info.flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
            info.stage = ShaderObject::stage_to_vk(m_stage_configs[i].stage);
            info.nextStage = m_stage_configs[i].next_stage;
            info.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
            info.codeSize = all_spirv[i].size() * sizeof(std::uint32_t);
            info.pCode = all_spirv[i].data();
            info.pName = "main";
            info.setLayoutCount = static_cast<std::uint32_t>(m_set_layouts.size());
            info.pSetLayouts = m_set_layouts.empty() ? nullptr : m_set_layouts.data();
            info.pushConstantRangeCount = static_cast<std::uint32_t>(m_push_constant_ranges.size());
            info.pPushConstantRanges = m_push_constant_ranges.empty() ? nullptr : m_push_constant_ranges.data();
            info.pSpecializationInfo = nullptr;
            create_infos.push_back(info);

            m_linked_stages.push_back(info.stage);
            m_stage_flags.push_back(info.stage);
        }

        // Create all linked shaders in a single call
        m_linked_shaders.resize(create_infos.size(), VK_NULL_HANDLE);

        auto result = fn.create_shaders(
            m_device->get_logical_device(),
            static_cast<std::uint32_t>(create_infos.size()),
            create_infos.data(),
            nullptr,
            m_linked_shaders.data());

        if (result != VK_SUCCESS) {
            // Clean up any partially created shaders
            for (auto &shader: m_linked_shaders) {
                if (shader != VK_NULL_HANDLE) {
                    fn.destroy_shader(m_device->get_logical_device(), shader, nullptr);
                    shader = VK_NULL_HANDLE;
                }
            }
            m_linked_shaders.clear();
            m_linked_stages.clear();
            throw std::runtime_error("Failed to create linked shaders for program '" + m_name + "'");
        }

        SUB_INFO("Created {} linked shaders for program '{}'", m_linked_shaders.size(), m_name);
    }

    auto ShaderProgram::destroy_linked_shaders() -> void {
        if (m_device == nullptr) {
            return;
        }

        const auto &fn = m_device->get_shader_object_fn();
        if (!fn.destroy_shader) {
            return;
        }

        for (auto &shader: m_linked_shaders) {
            if (shader != VK_NULL_HANDLE) {
                fn.destroy_shader(m_device->get_logical_device(), shader, nullptr);
                shader = VK_NULL_HANDLE;
            }
        }
        m_linked_shaders.clear();
        m_linked_stages.clear();
    }

    auto ShaderProgram::cleanup() -> void {
        if (m_mode == Mode::Linked) {
            destroy_linked_shaders();
        }

        // Unlinked shaders are cleaned up by unique_ptr destructors
        m_unlinked_shaders.clear();

        if (m_pipeline_layout != VK_NULL_HANDLE && m_device != nullptr) {
            ::vkDestroyPipelineLayout(m_device->get_logical_device(), m_pipeline_layout, nullptr);
            m_pipeline_layout = VK_NULL_HANDLE;
        }
    }

} // namespace flux
