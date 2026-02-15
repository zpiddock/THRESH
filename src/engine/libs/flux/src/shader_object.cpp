#include "flux/shader_object.hpp"
#include "flux/device.hpp"
#include "flux/shader_compiler.hpp"
#include "substratum/log.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace flux {

    ShaderObject::ShaderObject(const Config &config)
        : m_device{config.device}
          , m_filepath{config.filepath}
          , m_stage{config.stage}
          , m_next_stage{config.next_stage}
          , m_set_layouts{config.set_layouts}
          , m_push_constant_ranges{config.push_constant_ranges}
          , m_hot_reload_enabled{config.enable_hot_reload}
          , m_optimize{config.optimize}
          , m_throw_on_error{config.throw_on_error} {
        if (!std::filesystem::exists(m_filepath)) {
            m_last_error = "Shader file does not exist: " + m_filepath.string();
            if (m_throw_on_error) {
                SUB_FATAL("{}", m_last_error);
                throw std::runtime_error(m_last_error);
            }
            SUB_ERROR("{}", m_last_error);
            return;
        }

        try {
            m_spirv = load_spirv();
            create_shader_object(m_spirv);
            m_last_write_time = std::filesystem::last_write_time(m_filepath);
            SUB_INFO("Created shader object: {}", m_filepath.string());
        } catch (const std::exception &e) {
            m_last_error = e.what();
            if (m_throw_on_error) {
                throw;
            }
            SUB_ERROR("Shader object creation failed for '{}': {}", m_filepath.string(), m_last_error);
        }
    }

    ShaderObject::~ShaderObject() {
        destroy_shader_object();
    }

    ShaderObject::ShaderObject(ShaderObject &&other) noexcept
        : m_device{other.m_device}
          , m_shader{other.m_shader}
          , m_filepath{std::move(other.m_filepath)}
          , m_stage{other.m_stage}
          , m_next_stage{other.m_next_stage}
          , m_set_layouts{std::move(other.m_set_layouts)}
          , m_push_constant_ranges{std::move(other.m_push_constant_ranges)}
          , m_hot_reload_enabled{other.m_hot_reload_enabled}
          , m_optimize{other.m_optimize}
          , m_throw_on_error{other.m_throw_on_error}
          , m_last_write_time{other.m_last_write_time}
          , m_reload_callback{std::move(other.m_reload_callback)}
          , m_last_error{std::move(other.m_last_error)}
          , m_spirv{std::move(other.m_spirv)} {
        other.m_device = nullptr;
        other.m_shader = VK_NULL_HANDLE;
    }

    auto ShaderObject::operator=(ShaderObject &&other) noexcept -> ShaderObject & {
        if (this != &other) {
            destroy_shader_object();

            m_device = other.m_device;
            m_shader = other.m_shader;
            m_filepath = std::move(other.m_filepath);
            m_stage = other.m_stage;
            m_next_stage = other.m_next_stage;
            m_set_layouts = std::move(other.m_set_layouts);
            m_push_constant_ranges = std::move(other.m_push_constant_ranges);
            m_hot_reload_enabled = other.m_hot_reload_enabled;
            m_optimize = other.m_optimize;
            m_throw_on_error = other.m_throw_on_error;
            m_last_write_time = other.m_last_write_time;
            m_reload_callback = std::move(other.m_reload_callback);
            m_last_error = std::move(other.m_last_error);
            m_spirv = std::move(other.m_spirv);

            other.m_device = nullptr;
            other.m_shader = VK_NULL_HANDLE;
        }
        return *this;
    }

    auto ShaderObject::get_stage() const -> VkShaderStageFlagBits {
        return stage_to_vk(m_stage);
    }

    auto ShaderObject::reload() -> bool {
        try {
            SUB_INFO("Reloading shader object: {}", m_filepath.string());

            auto new_spirv = load_spirv();

            // Create new shader object before destroying old one
            VkShaderCreateInfoEXT create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
            create_info.flags = 0;
            create_info.stage = stage_to_vk(m_stage);
            create_info.nextStage = m_next_stage;
            create_info.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
            create_info.codeSize = new_spirv.size() * sizeof(std::uint32_t);
            create_info.pCode = new_spirv.data();
            create_info.pName = "main";
            create_info.setLayoutCount = static_cast<std::uint32_t>(m_set_layouts.size());
            create_info.pSetLayouts = m_set_layouts.empty() ? nullptr : m_set_layouts.data();
            create_info.pushConstantRangeCount = static_cast<std::uint32_t>(m_push_constant_ranges.size());
            create_info.pPushConstantRanges = m_push_constant_ranges.empty() ? nullptr : m_push_constant_ranges.data();
            create_info.pSpecializationInfo = nullptr;

            const auto &fn = m_device->get_shader_object_fn();
            VkShaderEXT new_shader = VK_NULL_HANDLE;
            auto result = fn.create_shaders(m_device->get_logical_device(), 1, &create_info, nullptr, &new_shader);

            if (result != VK_SUCCESS) {
                SUB_ERROR("Failed to create replacement shader object: {}", m_filepath.string());
                return false;
            }

            // Swap — old handle should be deferred-deleted by the callback owner
            auto old_shader = m_shader;
            m_shader = new_shader;
            m_spirv = std::move(new_spirv);
            m_last_write_time = std::filesystem::last_write_time(m_filepath);
            m_last_error.clear();

            SUB_INFO("Successfully reloaded shader object: {}", m_filepath.string());

            // Fire callback so the owner can queue old_shader for deferred deletion
            if (m_reload_callback) {
                m_reload_callback();
            }

            // If no callback is set, destroy the old shader immediately
            // (this is safe only if the GPU is not using it)
            if (!m_reload_callback && old_shader != VK_NULL_HANDLE) {
                fn.destroy_shader(m_device->get_logical_device(), old_shader, nullptr);
            }

            return true;
        } catch (const std::exception &e) {
            SUB_ERROR("Exception while reloading shader object: {}", e.what());
            return false;
        }
    }

    auto ShaderObject::check_and_reload() -> bool {
        if (!m_hot_reload_enabled) {
            return false;
        }

        if (!std::filesystem::exists(m_filepath)) {
            return false;
        }

        auto current_write_time = std::filesystem::last_write_time(m_filepath);
        if (current_write_time > m_last_write_time) {
            return reload();
        }

        return false;
    }

    auto ShaderObject::load_spirv() -> std::vector<std::uint32_t> {
        auto extension = m_filepath.extension().string();

        bool is_glsl = (extension == ".vert" || extension == ".frag"
                        || extension == ".comp" || extension == ".geom"
                        || extension == ".tesc" || extension == ".tese"
                        || extension == ".glsl");

        if (is_glsl) {
            ShaderCompiler compiler;
            ShaderCompiler::CompileOptions opts;
            opts.stage = static_cast<ShaderCompiler::Stage>(m_stage);
            opts.optimization = m_optimize
                                    ? ShaderCompiler::OptimizationLevel::Performance
                                    : ShaderCompiler::OptimizationLevel::None;
            opts.generate_debug_info = m_hot_reload_enabled;

            auto result = compiler.compile_file(m_filepath, opts);

            if (!result.success) {
                throw std::runtime_error("Shader compilation failed: " + result.error_message);
            }

            return std::move(result.spirv);
        }

        // Load pre-compiled SPIR-V
        std::ifstream file(m_filepath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open shader file: " + m_filepath.string());
        }

        auto file_size = static_cast<std::size_t>(file.tellg());
        std::vector<std::uint32_t> spirv(file_size / sizeof(std::uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char *>(spirv.data()), static_cast<std::streamsize>(file_size));
        file.close();

        return spirv;
    }

    auto ShaderObject::create_shader_object(const std::vector<std::uint32_t> &spirv) -> void {
        VkShaderCreateInfoEXT create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
        create_info.flags = 0; // Unlinked shader
        create_info.stage = stage_to_vk(m_stage);
        create_info.nextStage = m_next_stage;
        create_info.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
        create_info.codeSize = spirv.size() * sizeof(std::uint32_t);
        create_info.pCode = spirv.data();
        create_info.pName = "main";
        create_info.setLayoutCount = static_cast<std::uint32_t>(m_set_layouts.size());
        create_info.pSetLayouts = m_set_layouts.empty() ? nullptr : m_set_layouts.data();
        create_info.pushConstantRangeCount = static_cast<std::uint32_t>(m_push_constant_ranges.size());
        create_info.pPushConstantRanges = m_push_constant_ranges.empty() ? nullptr : m_push_constant_ranges.data();
        create_info.pSpecializationInfo = nullptr;

        const auto &fn = m_device->get_shader_object_fn();
        auto result = fn.create_shaders(m_device->get_logical_device(), 1, &create_info, nullptr, &m_shader);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader object for: " + m_filepath.string());
        }
    }

    auto ShaderObject::destroy_shader_object() -> void {
        if (m_shader != VK_NULL_HANDLE && m_device != nullptr) {
            const auto &fn = m_device->get_shader_object_fn();
            if (fn.destroy_shader) {
                fn.destroy_shader(m_device->get_logical_device(), m_shader, nullptr);
            }
            m_shader = VK_NULL_HANDLE;
        }
    }

    auto ShaderObject::stage_to_vk(Stage stage) -> VkShaderStageFlagBits {
        switch (stage) {
            case Stage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
            case Stage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case Stage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
            case Stage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
            case Stage::TessellationControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case Stage::TessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            default: return VK_SHADER_STAGE_VERTEX_BIT;
        }
    }

} // namespace flux
