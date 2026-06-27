//
// Created by Admin on 14/05/2026.
//

#pragma once
#include <vulkan/vulkan.hpp>
#include "horizon/window.hpp"
#include "SDL3/SDL_events.h"
#include "vkbackend/command_buffer.hpp"

namespace flux {

    class DearImGuiContext {

        struct ImGuiInitInfo {
            vk::Instance instance;
            vk::PhysicalDevice physical_device;
            vk::Device device;
            uint32_t queue_family_index;
            vk::Queue queue;
            vk::Format colour_format;
            uint32_t image_count;
            uint32_t min_image_count;
        };

        public:
            DearImGuiContext() = default;
            ~DearImGuiContext();

            DearImGuiContext(const DearImGuiContext&) = delete;
            auto operator=(const DearImGuiContext&) -> DearImGuiContext& = delete;

            auto init(const thresh::Window& window, const ImGuiInitInfo& info) -> void;

            auto shutdown() -> void;

            auto process_event(const SDL_Event& event) -> void;

            auto new_frame() -> bool;

            // Balances a new_frame() without producing draw data. Use when the frame
            // is being skipped (e.g. swapchain resize, OutOfDate acquire) so the next
            // new_frame() doesn't assert on the unfinished previous frame.
            auto discard_frame() -> void;

            auto record_draw_data(flux::CommandBuffer& cmd) -> void;

            bool m_enabled = false;

        private:
    };
} // flux
