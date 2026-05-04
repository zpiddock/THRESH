//
// Created by Admin on 04/05/2026.
//

#pragma once
#include <cstdint>
#include <vulkan/vulkan_raii.hpp>


class Texture {

    public:

        Texture();

    private:
        vk::raii::Image m_image = nullptr;
        vk::raii::DeviceMemory m_image_memory = nullptr;
};

