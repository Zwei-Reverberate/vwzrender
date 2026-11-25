#include <command.h>
#include <log.h>
#include <instance.h>

bool zvk::oneTimeSubmit(VkDevice device, VkQueue queue, const CommandPool& pool,
						const std::function<void(VkCommandBuffer)>& record)
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence fence = VK_NULL_HANDLE;
	VkResult result = vkCreateFence(device, &fenceInfo, nullptr, &fence);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("Failed to create fence: {} ({})", resultName(result), static_cast<int>(result));
		return false;
	}

	bool ok = true;
	const std::vector<VkCommandBuffer> buffers = pool.allocate(1);
	if (buffers.empty()) 
	{
		ok = false;
	}
	else 
	{
		const VkCommandBuffer cmd = buffers[0];

		// Recording starts: ONE_TIME_SUBMIT tells the driver "this buffer is invalid after one submit",
		// so the driver can apply one-shot optimizations(re-recording/reuse is misuse)
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		result = vkBeginCommandBuffer(cmd, &beginInfo);
		if (result != VK_SUCCESS) 
		{
			VWZ_LOG_ERROR("Failed to begin command buffer recording: {} ({})", resultName(result), static_cast<int>(result));
			ok = false;
		}
		else 
		{
			// Recording: caller logic(no GPU work happens during recording)
			record(cmd);

			result = vkEndCommandBuffer(cmd);
			if (result != VK_SUCCESS) 
			{
				VWZ_LOG_ERROR("Failed to end command buffer recording: {} ({})", resultName(result), static_cast<int>(result));
				ok = false;
			}
		}

		if (ok) 
		{
			// Submit: only now do the commands actually launch; the fence is registered in the submit as the completion signal
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmd;

			result = vkQueueSubmit(queue, 1, &submitInfo, fence);
			if (result != VK_SUCCESS) 
			{
				VWZ_LOG_ERROR("Queue submit failed: {} ({})", resultName(result), static_cast<int>(result));
				ok = false;
			}
		}

		if (ok) 
		{
			result = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
			if (result != VK_SUCCESS) 
			{
				VWZ_LOG_ERROR("Fence wait failed: {} ({})", resultName(result), static_cast<int>(result));
				ok = false;
			}
		}

		vkFreeCommandBuffers(device, pool.handle(), 1, &cmd);
	}

	vkDestroyFence(device, fence, nullptr);
	return ok;
}

zvk::CommandPool::~CommandPool()
{
	destroy();
}

void zvk::CommandPool::destroy()
{
	if (m_pool != VK_NULL_HANDLE) 
	{
		vkDestroyCommandPool(m_device, m_pool, nullptr);
		m_pool = VK_NULL_HANDLE;
	}
	m_device = VK_NULL_HANDLE;
}

bool zvk::CommandPool::create(VkDevice device, std::uint32_t queueFamily, VkCommandPoolCreateFlags flags)
{
	if (device == VK_NULL_HANDLE) 
	{
		VWZ_LOG_ERROR("CommandPool::create: invalid device");
		return false;
	}
	if (m_pool != VK_NULL_HANDLE) 
	{
		VWZ_LOG_WARN("CommandPool::create: pool already exists, destroying old pool first");
		destroy();
	}

	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = flags;
	createInfo.queueFamilyIndex = queueFamily;

	const VkResult result = vkCreateCommandPool(device, &createInfo, nullptr, &m_pool);
	if (result != VK_SUCCESS) 
	{
		m_pool = VK_NULL_HANDLE;
		VWZ_LOG_ERROR("Failed to create command pool: {} ({})", resultName(result), static_cast<int>(result));
		return false;
	}

	m_device = device;
	return true;
}

std::vector<VkCommandBuffer> zvk::CommandPool::allocate(std::uint32_t count) const
{
	if (m_pool == VK_NULL_HANDLE || count == 0) 
	{
		return {};
	}

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;

	std::vector<VkCommandBuffer> buffers(count, VK_NULL_HANDLE);
	const VkResult result = vkAllocateCommandBuffers(m_device, &allocInfo, buffers.data());
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("Failed to allocate command buffer: {} ({})", resultName(result), static_cast<int>(result));
		return {};
	}
	return buffers;
}

std::vector<VkCommandBuffer> zvk::CommandPool::allocateSecondary(std::uint32_t count) const
{
	if (m_pool == VK_NULL_HANDLE || count == 0) 
	{
		return {};
	}

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandBufferCount = count;

	std::vector<VkCommandBuffer> buffers(count, VK_NULL_HANDLE);
	const VkResult result = vkAllocateCommandBuffers(m_device, &allocInfo, buffers.data());
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("Failed to allocate secondary command buffer: {} ({})", resultName(result), static_cast<int>(result));
		return {};
	}
	return buffers;
}