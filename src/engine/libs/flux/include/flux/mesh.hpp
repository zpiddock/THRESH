#pragma once

#include "device.hpp"
#include "vertex.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#ifdef _WIN32
#ifdef FLUX_EXPORTS
#define FLUX_API __declspec(dllexport)
#else
#define FLUX_API __declspec(dllimport)
#endif
#else
#define FLUX_API
#endif

namespace flux {

    /**
     * Describes a sub-range of indices within a Mesh.
     * Each SubMesh corresponds to a distinct material assignment.
     */
    struct SubMesh {
        std::uint32_t index_offset = 0;   ///< First index into the mesh index buffer
        std::uint32_t index_count = 0;    ///< Number of indices in this submesh
        std::uint32_t material_index = 0; ///< Material slot for this submesh
    };

    /**
     * GPU mesh owning device-local vertex and index buffers.
     *
     * Created via the static factory `Mesh::create()`, which performs
     * staging-buffer upload via single-time commands.
     *
     * Usage:
     *   mesh->bind(cmd);
     *   mesh->draw_submesh(cmd, 0);
     */
    class FLUX_API Mesh {
    public:
        ~Mesh();

        Mesh(const Mesh &) = delete;
        auto operator=(const Mesh &) -> Mesh & = delete;

        Mesh(Mesh &&other) noexcept;
        auto operator=(Mesh &&other) noexcept -> Mesh &;

        /**
         * Create a mesh from vertex and index data.
         * Uploads to device-local memory via staging buffer.
         * @param device Vulkan device (must have command pool set)
         * @param vertices Vertex data
         * @param indices Index data (uint32)
         * @param submeshes Submesh descriptors
         * @return Mesh ready for rendering, or nullptr on failure
         */
        [[nodiscard]] static auto create(
            Device &device,
            std::span<const Vertex> vertices,
            std::span<const std::uint32_t> indices,
            std::vector<SubMesh> submeshes
        ) -> std::unique_ptr<Mesh>;

        /**
         * Bind vertex and index buffers to the command buffer.
         */
        auto bind(VkCommandBuffer cmd) const -> void;

        /**
         * Draw a specific submesh using vkCmdDrawIndexed.
         * @param cmd Active command buffer (must have pipeline bound)
         * @param submesh_index Index into the submesh array
         */
        auto draw_submesh(VkCommandBuffer cmd, std::uint32_t submesh_index) const -> void;

        /**
         * Draw all submeshes sequentially.
         */
        auto draw_all(VkCommandBuffer cmd) const -> void;

        [[nodiscard]] auto get_submeshes() const -> std::span<const SubMesh> { return m_submeshes; }
        [[nodiscard]] auto get_vertex_count() const -> std::uint32_t { return m_vertex_count; }
        [[nodiscard]] auto get_index_count() const -> std::uint32_t { return m_index_count; }

    private:
        Mesh() = default;

        VmaAllocator m_allocator = nullptr;

        VkBuffer m_vertex_buffer = VK_NULL_HANDLE;
        VmaAllocation m_vertex_alloc = nullptr;

        VkBuffer m_index_buffer = VK_NULL_HANDLE;
        VmaAllocation m_index_alloc = nullptr;

        std::uint32_t m_vertex_count = 0;
        std::uint32_t m_index_count = 0;
        std::vector<SubMesh> m_submeshes;
    };

} // namespace flux
