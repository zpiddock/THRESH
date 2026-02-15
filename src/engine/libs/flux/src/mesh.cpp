#include "flux/mesh.hpp"
#include "substratum/log.hpp"

#include <cstring>
#include <stdexcept>

namespace flux {

    Mesh::~Mesh() {
        if (m_vertex_buffer != VK_NULL_HANDLE && m_allocator != nullptr) {
            ::vmaDestroyBuffer(m_allocator, m_vertex_buffer, m_vertex_alloc);
        }
        if (m_index_buffer != VK_NULL_HANDLE && m_allocator != nullptr) {
            ::vmaDestroyBuffer(m_allocator, m_index_buffer, m_index_alloc);
        }
    }

    Mesh::Mesh(Mesh &&other) noexcept
        : m_allocator{other.m_allocator}
        , m_vertex_buffer{other.m_vertex_buffer}
        , m_vertex_alloc{other.m_vertex_alloc}
        , m_index_buffer{other.m_index_buffer}
        , m_index_alloc{other.m_index_alloc}
        , m_vertex_count{other.m_vertex_count}
        , m_index_count{other.m_index_count}
        , m_submeshes{std::move(other.m_submeshes)} {
        other.m_allocator = nullptr;
        other.m_vertex_buffer = VK_NULL_HANDLE;
        other.m_vertex_alloc = nullptr;
        other.m_index_buffer = VK_NULL_HANDLE;
        other.m_index_alloc = nullptr;
        other.m_vertex_count = 0;
        other.m_index_count = 0;
    }

    auto Mesh::operator=(Mesh &&other) noexcept -> Mesh & {
        if (this != &other) {
            // Destroy existing resources
            if (m_vertex_buffer != VK_NULL_HANDLE && m_allocator != nullptr) {
                ::vmaDestroyBuffer(m_allocator, m_vertex_buffer, m_vertex_alloc);
            }
            if (m_index_buffer != VK_NULL_HANDLE && m_allocator != nullptr) {
                ::vmaDestroyBuffer(m_allocator, m_index_buffer, m_index_alloc);
            }

            m_allocator = other.m_allocator;
            m_vertex_buffer = other.m_vertex_buffer;
            m_vertex_alloc = other.m_vertex_alloc;
            m_index_buffer = other.m_index_buffer;
            m_index_alloc = other.m_index_alloc;
            m_vertex_count = other.m_vertex_count;
            m_index_count = other.m_index_count;
            m_submeshes = std::move(other.m_submeshes);

            other.m_allocator = nullptr;
            other.m_vertex_buffer = VK_NULL_HANDLE;
            other.m_vertex_alloc = nullptr;
            other.m_index_buffer = VK_NULL_HANDLE;
            other.m_index_alloc = nullptr;
            other.m_vertex_count = 0;
            other.m_index_count = 0;
        }
        return *this;
    }

    auto Mesh::create(
        Device &device,
        std::span<const Vertex> vertices,
        std::span<const std::uint32_t> indices,
        std::vector<SubMesh> submeshes
    ) -> std::unique_ptr<Mesh> {
        if (vertices.empty() || indices.empty()) {
            SUB_ERROR("Mesh::create called with empty vertex or index data");
            return nullptr;
        }

        auto mesh = std::unique_ptr<Mesh>(new Mesh());
        mesh->m_allocator = device.get_allocator();
        mesh->m_vertex_count = static_cast<std::uint32_t>(vertices.size());
        mesh->m_index_count = static_cast<std::uint32_t>(indices.size());
        mesh->m_submeshes = std::move(submeshes);

        VkDeviceSize vertex_size = vertices.size_bytes();
        VkDeviceSize index_size = indices.size_bytes();

        // ── Create staging buffers ──────────────────────────────────────────

        auto create_staging = [&](VkDeviceSize size) -> std::pair<VkBuffer, VmaAllocation> {
            VkBufferCreateInfo buf_info{};
            buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buf_info.size = size;
            buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo alloc_info{};
            alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
            alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                               VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation alloc = nullptr;

            if (::vmaCreateBuffer(device.get_allocator(), &buf_info, &alloc_info, &buffer, &alloc, nullptr) != VK_SUCCESS) {
                SUB_ERROR("Failed to create staging buffer ({} bytes)", size);
                return {VK_NULL_HANDLE, nullptr};
            }
            return {buffer, alloc};
        };

        auto create_device_local = [&](VkDeviceSize size, VkBufferUsageFlags extra_usage) -> std::pair<VkBuffer, VmaAllocation> {
            VkBufferCreateInfo buf_info{};
            buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buf_info.size = size;
            buf_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | extra_usage;
            buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo alloc_info{};
            alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation alloc = nullptr;

            if (::vmaCreateBuffer(device.get_allocator(), &buf_info, &alloc_info, &buffer, &alloc, nullptr) != VK_SUCCESS) {
                SUB_ERROR("Failed to create device-local buffer ({} bytes)", size);
                return {VK_NULL_HANDLE, nullptr};
            }
            return {buffer, alloc};
        };

        // Create staging buffers
        auto [vb_staging, vb_staging_alloc] = create_staging(vertex_size);
        auto [ib_staging, ib_staging_alloc] = create_staging(index_size);

        if (vb_staging == VK_NULL_HANDLE || ib_staging == VK_NULL_HANDLE) {
            if (vb_staging != VK_NULL_HANDLE) ::vmaDestroyBuffer(device.get_allocator(), vb_staging, vb_staging_alloc);
            if (ib_staging != VK_NULL_HANDLE) ::vmaDestroyBuffer(device.get_allocator(), ib_staging, ib_staging_alloc);
            return nullptr;
        }

        // Copy data into staging buffers
        VmaAllocationInfo vb_info{};
        VmaAllocationInfo ib_info{};
        ::vmaGetAllocationInfo(device.get_allocator(), vb_staging_alloc, &vb_info);
        ::vmaGetAllocationInfo(device.get_allocator(), ib_staging_alloc, &ib_info);

        std::memcpy(vb_info.pMappedData, vertices.data(), vertex_size);
        std::memcpy(ib_info.pMappedData, indices.data(), index_size);

        // Create device-local buffers
        auto [vb_device, vb_device_alloc] = create_device_local(vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        auto [ib_device, ib_device_alloc] = create_device_local(index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        if (vb_device == VK_NULL_HANDLE || ib_device == VK_NULL_HANDLE) {
            ::vmaDestroyBuffer(device.get_allocator(), vb_staging, vb_staging_alloc);
            ::vmaDestroyBuffer(device.get_allocator(), ib_staging, ib_staging_alloc);
            if (vb_device != VK_NULL_HANDLE) ::vmaDestroyBuffer(device.get_allocator(), vb_device, vb_device_alloc);
            if (ib_device != VK_NULL_HANDLE) ::vmaDestroyBuffer(device.get_allocator(), ib_device, ib_device_alloc);
            return nullptr;
        }

        // ── Upload via single-time commands ─────────────────────────────────

        auto cmd = device.begin_single_time_commands();

        VkBufferCopy vb_copy{};
        vb_copy.size = vertex_size;
        ::vkCmdCopyBuffer(cmd, vb_staging, vb_device, 1, &vb_copy);

        VkBufferCopy ib_copy{};
        ib_copy.size = index_size;
        ::vkCmdCopyBuffer(cmd, ib_staging, ib_device, 1, &ib_copy);

        device.end_single_time_commands(cmd);

        // Clean up staging buffers
        ::vmaDestroyBuffer(device.get_allocator(), vb_staging, vb_staging_alloc);
        ::vmaDestroyBuffer(device.get_allocator(), ib_staging, ib_staging_alloc);

        mesh->m_vertex_buffer = vb_device;
        mesh->m_vertex_alloc = vb_device_alloc;
        mesh->m_index_buffer = ib_device;
        mesh->m_index_alloc = ib_device_alloc;

        SUB_DEBUG("Created mesh: {} vertices, {} indices, {} submeshes",
                  mesh->m_vertex_count, mesh->m_index_count, mesh->m_submeshes.size());

        return mesh;
    }

    auto Mesh::bind(VkCommandBuffer cmd) const -> void {
        VkBuffer buffers[] = {m_vertex_buffer};
        VkDeviceSize offsets[] = {0};
        ::vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        ::vkCmdBindIndexBuffer(cmd, m_index_buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    auto Mesh::draw_submesh(VkCommandBuffer cmd, std::uint32_t submesh_index) const -> void {
        if (submesh_index >= m_submeshes.size()) {
            SUB_ERROR("Submesh index {} out of range ({})", submesh_index, m_submeshes.size());
            return;
        }

        const auto &sub = m_submeshes[submesh_index];
        ::vkCmdDrawIndexed(cmd, sub.index_count, 1, sub.index_offset, 0, 0);
    }

    auto Mesh::draw_all(VkCommandBuffer cmd) const -> void {
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(m_submeshes.size()); ++i) {
            draw_submesh(cmd, i);
        }
    }

} // namespace flux
