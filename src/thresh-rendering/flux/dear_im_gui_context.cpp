//
// Created by Admin on 14/05/2026.
//

#include "dear_im_gui_context.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

namespace flux {
    DearImGuiContext::~DearImGuiContext() {
    }

    auto DearImGuiContext::init(const thresh::Window& window, const ImGuiInitInfo& info) -> void {

        IMGUI_CHECKVERSION();

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForVulkan(window.getWindow());

        vk::PipelineRenderingCreateInfo create_info = {
            .sType = vk::StructureType::ePipelineRenderingCreateInfo,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &info.colour_format
        };

        ImGui_ImplVulkan_PipelineInfo pipeline_info = {
            .MSAASamples                 = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
            .PipelineRenderingCreateInfo = create_info,
        };

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = info.instance;
        init_info.PhysicalDevice = info.physical_device;
        init_info.Device = info.device;
        init_info.QueueFamily = info.queue_family_index;
        init_info.Queue = info.queue;
        init_info.DescriptorPool = VK_NULL_HANDLE;          // ImGui manages its own pool
        init_info.DescriptorPoolSize = 64;                  // headroom for ImGui_ImplVulkan_AddTexture calls later
        init_info.MinImageCount = info.min_image_count;
        init_info.ImageCount = info.image_count;
        init_info.UseDynamicRendering = true;
        init_info.PipelineInfoMain = pipeline_info;
        ImGui_ImplVulkan_Init(&init_info);
    }

    auto DearImGuiContext::shutdown() -> void {

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    auto DearImGuiContext::process_event(const SDL_Event& event) -> void {

        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    auto DearImGuiContext::new_frame() -> bool {

        if (m_enabled) {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
        }
        return m_enabled;
    }

    auto DearImGuiContext::discard_frame() -> void {

        if (!m_enabled) { return; }
        ImGui::EndFrame();
    }

    auto DearImGuiContext::record_draw_data(flux::CommandBuffer& cmd) -> void {

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd.raw());
    }
} // flux