#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <functional>
#include <vector>
#include <copymovepolicy.h>

namespace zvk
{
	class CommandPool : public vwz::core::CopyMovePolicy<false>
	{
	public:
		CommandPool() = default;
		~CommandPool();

		bool create(VkDevice device, std::uint32_t queueFamily,
		VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		void destroy();
		bool isValid() const { return m_pool != VK_NULL_HANDLE; }
		VkCommandPool handle() const { return m_pool; }

		std::vector<VkCommandBuffer> allocate(std::uint32_t count) const;
		std::vector<VkCommandBuffer> allocateSecondary(std::uint32_t count) const;

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkCommandPool m_pool = VK_NULL_HANDLE;
	};

	bool oneTimeSubmit(VkDevice device, VkQueue queue, const CommandPool& pool, 
					   const std::function<void(VkCommandBuffer)>& record);
}