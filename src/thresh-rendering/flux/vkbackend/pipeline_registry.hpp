//
// Created by Admin on 21/06/2026.
//

#pragma once
#include <string>
#include <unordered_map>

#include "vulkan_pipeline.hpp"

namespace flux {
    class ThreshVkDevice;

    class PipelineRegistry {

        public:
            explicit PipelineRegistry(ThreshVkDevice& device) : m_device(device) {}

            auto register_pipeline(const std::string& name, const PipelineContext& ctx) -> ThreshVkPipeline*;

            auto get(const std::string& name) -> ThreshVkPipeline*;

            auto register_geometry_pipeline(vk::Format colour, vk::Format depth) -> void;

            auto register_composite_pipeline(vk::Format swapchain_colour) -> void;

        private:
            ThreshVkDevice&                                                     m_device;
            std::unordered_map<std::string, std::unique_ptr<ThreshVkPipeline>> m_pipelines;
    };
} // flux
