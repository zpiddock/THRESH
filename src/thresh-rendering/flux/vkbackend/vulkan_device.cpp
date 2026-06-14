//
// Created by Admin on 06/05/2026.
//

#include "vulkan_device.hpp"

#include "buffer.hpp"
#include "substratum/log.hpp"

namespace flux {
    ThreshVkDevice::ThreshVkDevice(const vk::raii::Instance& instance, const vk::SurfaceKHR& surface) {

        pick_suitable_device(instance);
        query_heap_properties();
        create_logical_device(surface);
        create_command_pool();
        validate_heap_strides();
    }

    auto ThreshVkDevice::transition_image_layout(const vk::raii::Image& image, vk::ImageLayout old_layout,
        vk::ImageLayout new_layout, vk::ImageAspectFlags aspect_flags) -> void {

        const auto command_buffer = begin_single_time_commands();

        vk::ImageMemoryBarrier barrier{
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .image = image,
            .subresourceRange = {
                .aspectMask = aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vk::PipelineStageFlags source_stage;
        vk::PipelineStageFlags destination_stage;

        if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            source_stage      = vk::PipelineStageFlagBits::eTopOfPipe;
            destination_stage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            source_stage      = vk::PipelineStageFlagBits::eTransfer;
            destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            SUB_FATAL("unsupported layout transition!");
        }

        command_buffer.pipelineBarrier(source_stage, destination_stage, {}, {}, nullptr, barrier);

        end_single_time_commands(command_buffer);
    }

    auto ThreshVkDevice::find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) -> uint32_t {

        vk::PhysicalDeviceMemoryProperties memory_properties = m_physical_device.getMemoryProperties();
        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        SUB_FATAL("Failed to find suitable memory type!");
        std::unreachable();
    }

    auto ThreshVkDevice::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties) -> std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> {

            const vk::BufferCreateInfo buffer_info{
                .size = size,
                .usage = usage,
                .sharingMode = vk::SharingMode::eExclusive
              };

            auto buffer = vk::raii::Buffer(m_device, buffer_info);

            auto mem_reqs = buffer.getMemoryRequirements();

            vk::MemoryAllocateInfo alloc_info{
                .allocationSize = mem_reqs.size,
                .memoryTypeIndex = find_memory_type(mem_reqs.memoryTypeBits, properties)
            };

            auto buffer_memory = vk::raii::DeviceMemory(m_device, alloc_info);

            buffer.bindMemory(*buffer_memory, 0);

            return {std::move(buffer), std::move(buffer_memory)};
    }

    auto ThreshVkDevice::upload_device_local(std::span<const std::byte> data, vk::BufferUsageFlags usage,
        const char* debug_name) -> flux::Buffer {

        flux::Buffer staging_buffer(*this, {
            .size = data.size(),
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .memory = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            .persistent_map = true,
            .debug_name = "Staging Buffer"
        });
        std::memcpy(staging_buffer.mapped().data(), data.data(), data.size());

        Buffer result(*this, {
            .size = data.size(),
            .usage = usage | vk::BufferUsageFlagBits::eTransferDst,
            .memory = vk::MemoryPropertyFlagBits::eDeviceLocal,
            .debug_name = debug_name
        });

        const auto cmd = begin_single_time_commands();
        cmd.copyBuffer(staging_buffer.handle(), result.handle(), vk::BufferCopy{.size = data.size()});
        end_single_time_commands(cmd);

        return result;
    }

    auto ThreshVkDevice::begin_single_time_commands() -> vk::raii::CommandBuffer {

        vk::CommandBufferAllocateInfo alloc_info{
            .commandPool = m_command_pool,
            .level       = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        vk::raii::CommandBuffer command_buffer = std::move(m_device.allocateCommandBuffers(alloc_info).front());

        command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        return command_buffer;
    }

    auto ThreshVkDevice::end_single_time_commands(const vk::raii::CommandBuffer& command_buffer) -> void {

        command_buffer.end();

        m_graphics_queue.submit(vk::SubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers    = &*command_buffer},
        nullptr);

        m_graphics_queue.waitIdle();
    }

    auto ThreshVkDevice::copy_buffer(const vk::raii::Buffer& src_buffer, vk::raii::Buffer& dst_buffer,
        vk::DeviceSize size) -> void {

        const auto command_buffer = begin_single_time_commands();

        command_buffer.copyBuffer(*src_buffer, *dst_buffer, vk::BufferCopy(0, 0, size));

        end_single_time_commands(command_buffer);
    }

    auto ThreshVkDevice::copy_buffer_to_image(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width,
        uint32_t height) -> void {

        const auto command_buffer = begin_single_time_commands();

        vk::BufferImageCopy region {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = vk::ImageSubresourceLayers{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1}
        };
        command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

        end_single_time_commands(command_buffer);
    }

    auto ThreshVkDevice::create_image(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling_mode,
        vk::ImageUsageFlags usage_flags,
        vk::MemoryPropertyFlags memory_props) -> std::pair<vk::raii::Image, vk::raii::DeviceMemory> {

        vk::raii::Image texture_image_temp({});
        vk::raii::DeviceMemory texture_image_memory_temp({});

        vk::ImageCreateInfo image_info{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = {width, height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = tiling_mode,
            .usage = usage_flags,
            .sharingMode = vk::SharingMode::eExclusive,
        };
        texture_image_temp = vk::raii::Image(m_device, image_info);
        texture_image_memory_temp = vk::raii::DeviceMemory(m_device, vk::MemoryAllocateInfo{
            .allocationSize = texture_image_temp.getMemoryRequirements().size,
            .memoryTypeIndex = find_memory_type(texture_image_temp.getMemoryRequirements().memoryTypeBits, memory_props)
        });
        texture_image_temp.bindMemory(*texture_image_memory_temp, 0);

        return {std::move(texture_image_temp), std::move(texture_image_memory_temp)};
    }

    auto ThreshVkDevice::create_image_view(const vk::Image& image, vk::Format format,
        vk::ImageAspectFlags flags) -> vk::raii::ImageView {

        vk::ImageViewCreateInfo view_info{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = flags,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
        };

        return {m_device, view_info};
    }

    auto ThreshVkDevice::find_supported_format(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
        vk::FormatFeatureFlags features) -> vk::Format {

        for (const auto& candidate : candidates) {
            auto format_properties = m_physical_device.getFormatProperties(candidate);

            if (tiling == vk::ImageTiling::eLinear && (format_properties.linearTilingFeatures & features) == features) {
                return candidate;
            }
            if (tiling == vk::ImageTiling::eOptimal && (format_properties.optimalTilingFeatures & features) == features) {
                return candidate;
            }
        }
        SUB_FATAL("Failed to find supported format!");
        std::unreachable();
    }

    auto ThreshVkDevice::find_depth_format() -> vk::Format {

        return find_supported_format(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    auto ThreshVkDevice::has_stencil_component(vk::Format format) -> bool {

        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    auto ThreshVkDevice::pick_suitable_device(const vk::raii::Instance& instance) -> void {
        auto devices = instance.enumeratePhysicalDevices();

        auto const device_iterator = std::ranges::find_if(devices, [&](const auto& device) {
            return is_device_suitable(device);
        });
        if (device_iterator == devices.end()) {
            throw std::runtime_error("No suitable GPU with Vulkan 1.4 Support found");
        }
        m_physical_device = *device_iterator;
        SUB_TRACE("Using GPU: {}", m_physical_device.getProperties2().properties.deviceName.data());
    }

    auto ThreshVkDevice::create_logical_device(const vk::SurfaceKHR& surface) -> void {
        const auto queue_family_properties = m_physical_device.getQueueFamilyProperties();

        for (uint32_t qfpIndex = 0; qfpIndex < queue_family_properties.size(); qfpIndex++) {
            if ((queue_family_properties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_physical_device.getSurfaceSupportKHR(qfpIndex, surface)) {
                // found a queue family that supports both graphics and present
                m_queue_family_index = qfpIndex;
                break;
            }
        }
        if (m_queue_family_index == ~0u) {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }
        SUB_DEBUG("Selected queue family {} (graphics+present)", m_queue_family_index);

        vk::StructureChain<
                    vk::PhysicalDeviceFeatures2,
                    vk::PhysicalDeviceVulkan11Features,
                    vk::PhysicalDeviceVulkan12Features,
                    vk::PhysicalDeviceVulkan13Features,
                    vk::PhysicalDeviceVulkan14Features,
                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                    vk::PhysicalDeviceDescriptorHeapFeaturesEXT,
                    vk::PhysicalDeviceShaderUntypedPointersFeaturesKHR>
                features{
                    { // PhysicalDeviceFeatures2
                        .features = {
                            .samplerAnisotropy = true,
                        }
                    },
                    { // Vulkan 1.1 Features
                        .shaderDrawParameters = true,
                    },
                    { // Vulkan 1.2 Features
                        .scalarBlockLayout = true,
                        .bufferDeviceAddress = true,
                    },
                    { // Vulkan 1.3 Features
                        .synchronization2 = true,
                        .dynamicRendering = true
                    },
                    { // Vulkan 1.4 Features
                        .maintenance5 = true
                    },
                    { // PhysicalDeviceExtendedDynamicStateFeaturesEXT
                        .extendedDynamicState = true
                    },
                    { // PhysicalDeviceDescriptorHeapFeaturesEXT
                        .descriptorHeap = true
                    },
                    { // PhysicalDeviceShaderUntypedPointersFeaturesKHR
                        .shaderUntypedPointers = true
                    }
                };

        float                     queue_priority = .5f;
        vk::DeviceQueueCreateInfo queue_info{
            .queueFamilyIndex = m_queue_family_index,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority
        };

        vk::DeviceCreateInfo device_info{
            .pNext                   = &features.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount    = 1,
            .pQueueCreateInfos       = &queue_info,
            .enabledExtensionCount   = static_cast<uint32_t>(m_required_device_extensions.size()),
            .ppEnabledExtensionNames = m_required_device_extensions.data()
        };

        m_device         = vk::raii::Device(m_physical_device, device_info);
        m_graphics_queue = vk::raii::Queue(m_device, m_queue_family_index, 0);
        SUB_INFO("Vulkan logical device created (extensions={})", m_required_device_extensions.size());
    }

    auto ThreshVkDevice::is_device_suitable(const vk::PhysicalDevice& device) -> bool {
        const auto device_properties = device.getProperties();
        const auto support_VK1_4 = device_properties.apiVersion >= vk::ApiVersion14;

        auto queue_families   = device.getQueueFamilyProperties();
        auto support_graphics = std::ranges::any_of(queue_families, [](const auto& family) {
            return !!(family.queueFlags & vk::QueueFlagBits::eGraphics);
        });

        auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions =
                std::ranges::all_of(m_required_device_extensions,
                                    [&availableDeviceExtensions](auto const& requiredDeviceExtension) {
                                        return std::ranges::any_of(availableDeviceExtensions,
                                                                   [requiredDeviceExtension](auto const& available) {
                                                                       return strcmp(available.extensionName,
                                                                                     requiredDeviceExtension) == 0;
                                                                   });
                                    });

        auto features = device.getFeatures2<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceVulkan11Features,
                vk::PhysicalDeviceVulkan12Features,
                vk::PhysicalDeviceVulkan13Features,
                vk::PhysicalDeviceVulkan14Features,
                vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                vk::PhysicalDeviceDescriptorHeapFeaturesEXT,
                vk::PhysicalDeviceShaderUntypedPointersFeaturesKHR>();

        const auto& f10 = features.get<vk::PhysicalDeviceFeatures2>().features;
        const auto& f11 = features.get<vk::PhysicalDeviceVulkan11Features>();
        const auto& f12 = features.get<vk::PhysicalDeviceVulkan12Features>();
        const auto& f13 = features.get<vk::PhysicalDeviceVulkan13Features>();
        const auto& f14 = features.get<vk::PhysicalDeviceVulkan14Features>();
        const auto& fds = features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        const auto& fheap = features.get<vk::PhysicalDeviceDescriptorHeapFeaturesEXT>();
        const auto& fptr  = features.get<vk::PhysicalDeviceShaderUntypedPointersFeaturesKHR>();

        bool supportsRequiredFeatures =
                f10.samplerAnisotropy &&
                f11.shaderDrawParameters &&
                f12.scalarBlockLayout &&
                f12.bufferDeviceAddress &&
                f13.synchronization2 &&
                f13.dynamicRendering &&
                f14.maintenance5 &&
                fds.extendedDynamicState &&
                fheap.descriptorHeap &&
                fptr.shaderUntypedPointers;

        return support_VK1_4 && support_graphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }

    auto ThreshVkDevice::create_command_pool() -> void {
        const vk::CommandPoolCreateInfo pool_info{
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = m_queue_family_index
        };
        m_command_pool = vk::raii::CommandPool(m_device, pool_info);
        SUB_TRACE("Command pool created (queue family {})", m_queue_family_index);
    }

    auto ThreshVkDevice::query_heap_properties() -> void {

        auto chain = m_physical_device.getProperties2<
            vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceDescriptorHeapPropertiesEXT>();

        m_heap_properties = chain.get<vk::PhysicalDeviceDescriptorHeapPropertiesEXT>();
        // Copied out of `chain`, which dies at end of scope — its pNext now dangles. We only
        // ever read scalar fields off m_heap_properties, so null it to kill the stale pointer.
        m_heap_properties.pNext = nullptr;

        SUB_INFO("Descriptor heap: descSize img={}B buf={}B smp={} | align img={} buf={} smp={} | "
                 "maxHeap res={}B smp={}B | minReserved res={} smp={} | maxPushData={}B",
                 m_heap_properties.imageDescriptorSize,
                 m_heap_properties.bufferDescriptorSize,
                 m_heap_properties.samplerDescriptorSize,
                 m_heap_properties.imageDescriptorAlignment,
                 m_heap_properties.bufferDescriptorAlignment,
                 m_heap_properties.samplerDescriptorAlignment,
                 m_heap_properties.maxResourceHeapSize,
                 m_heap_properties.maxSamplerHeapSize,
                 m_heap_properties.minResourceHeapReservedRange,
                 m_heap_properties.minSamplerHeapReservedRange,
                 m_heap_properties.maxPushDataSize);
    }

    auto ThreshVkDevice::validate_heap_strides() -> void {

        const auto& p = m_heap_properties;
        // THE contract between the offline cook (-spirv-*-heap-stride) and this device.
        // If any of these fire: raise THRESH_*_HEAP_STRIDE in CMake and re-cook. One number.
        constexpr vk::DeviceSize rstride = THRESH_RESOURCE_HEAP_STRIDE;
        constexpr vk::DeviceSize sstride = THRESH_SAMPLER_HEAP_STRIDE;

        if (rstride < p.imageDescriptorSize  || rstride < p.bufferDescriptorSize ||
            rstride % p.imageDescriptorAlignment != 0 || rstride % p.bufferDescriptorAlignment != 0) {
            SUB_FATAL("Resource heap stride {} incompatible with device (img {}/{} buf {}/{})",
                      rstride, p.imageDescriptorSize, p.imageDescriptorAlignment,
                      p.bufferDescriptorSize, p.bufferDescriptorAlignment);
            }
        if (sstride < p.samplerDescriptorSize || sstride % p.samplerDescriptorAlignment != 0) {
            SUB_FATAL("Sampler heap stride {} incompatible with device (smp {}/{})",
                      sstride, p.samplerDescriptorSize, p.samplerDescriptorAlignment);
        }
        SUB_INFO("Descriptor heap: img={}B buf={}B smp={}B, maxPushData={}B, minReserved r={} s={}",
                 p.imageDescriptorSize, p.bufferDescriptorSize, p.samplerDescriptorSize,
                 p.maxPushDataSize, p.minResourceHeapReservedRange, p.minSamplerHeapReservedRange);
    }
} // flux