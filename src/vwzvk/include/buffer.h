#pragma once
#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <copymovepolicy.h>

namespace zvk
{
	class CommandPool;

	std::uint32_t findMemoryType(VkPhysicalDevice physicalDevice, std::uint32_t typeFilter, VkMemoryPropertyFlags properties);

	class Buffer : public vwz::core::CopyMovePolicy<true>
	{
	public:
		Buffer() = default;
		~Buffer();

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