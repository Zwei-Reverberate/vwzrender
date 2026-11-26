#pragma once
#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <utility>
#include <copymovepolicy.h>

namespace zvk
{
	class CommandPool;

	std::uint32_t findMemoryType(VkPhysicalDevice physicalDevice, std::uint32_t typeFilter, VkMemoryPropertyFlags properties);

	class Buffer : private vwz::core::CopyMovePolicy<Buffer, true>
	{
	public:
		Buffer() = default;
		~Buffer();

		Buffer(Buffer&& other) noexcept
			: m_device(std::exchange(other.m_device, VK_NULL_HANDLE))
			, m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE))
			, m_memory(std::exchange(other.m_memory, VK_NULL_HANDLE))
			, m_size(std::exchange(other.m_size, 0))
			, m_mapped(std::exchange(other.m_mapped, nullptr))
		{
		}

		Buffer& operator=(Buffer&& other) noexcept
		{
			if (this != &other)
			{
				destroy();
				m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
				m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
				m_memory = std::exchange(other.m_memory, VK_NULL_HANDLE);
				m_size = std::exchange(other.m_size, 0);
				m_mapped = std::exchange(other.m_mapped, nullptr);
			}
			return *this;
		}

		bool create(VkDevice device, VkPhysicalDevice physicalDevice,
			VkDeviceSize size, VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties);

		bool createWithStaging(VkDevice device, VkPhysicalDevice physicalDevice,
			VkQueue queue, const CommandPool& pool, const void* data, VkDeviceSize size,
			VkBufferUsageFlags usage);
		void destroy();
		bool isValid() const { return m_buffer != VK_NULL_HANDLE; }
		VkBuffer handle() const { return m_buffer; }
		VkDeviceSize size() const { return m_size; }

		void* map();
		void unmap();

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkDeviceSize m_size = 0;
		void* m_mapped = nullptr;
	};
}