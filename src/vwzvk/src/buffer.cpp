#include <buffer.h>
#include <command.h>
#include <instance.h>
#include <log.h>

std::uint32_t zvk::findMemoryType(VkPhysicalDevice physicalDevice,
	std::uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProps{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	for (std::uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		const bool allowed = (typeFilter & (1u << i)) != 0;
		const bool satisfied = (memProps.memoryTypes[i].propertyFlags & properties) == properties;
		if (allowed && satisfied)
		{
			return i;
		}
	}
	return 0xFFFFFFFFu;
}

zvk::Buffer::~Buffer()
{
	destroy();
}

bool zvk::Buffer::create(VkDevice device, VkPhysicalDevice physicalDevice,
					     VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || size == 0) 
	{
		VWZ_LOG_ERROR("Buffer::create: invalid parameters(device/physicalDevice is null or size=0)");
		return false;
	}
	if (m_buffer != VK_NULL_HANDLE) 
	{
		VWZ_LOG_WARN("Buffer::create: buffer already exists, destroying old buffer first");
		destroy();
	}
	m_device = device;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // exclusive use by a single queue family

	VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("Failed to create buffer: {} ({})", resultName(result), static_cast<int>(result));
		m_device = VK_NULL_HANDLE;
		return false;
	}

	VkMemoryRequirements requirements{};
	vkGetBufferMemoryRequirements(device, m_buffer, &requirements);

	const std::uint32_t typeIndex = findMemoryType(physicalDevice, requirements.memoryTypeBits, properties);
	if (typeIndex == 0xFFFFFFFFu) 
	{
		VWZ_LOG_ERROR("No memory type satisfies properties(0x{:X})", static_cast<unsigned>(properties));
		destroy();
		return false;
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = requirements.size;
	allocInfo.memoryTypeIndex = typeIndex;

	result = vkAllocateMemory(device, &allocInfo, nullptr, &m_memory);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("Failed to allocate device memory: {} ({})", resultName(result), static_cast<int>(result));
		destroy();
		return false;
	}

	result = vkBindBufferMemory(device, m_buffer, m_memory, 0);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("Failed to bind buffer memory: {} ({})", resultName(result), static_cast<int>(result));
		destroy();
		return false;
	}

	m_size = size;
	return true;
}

bool zvk::Buffer::createWithStaging(VkDevice device, VkPhysicalDevice physicalDevice,
									VkQueue queue, const CommandPool& pool, const void* data, VkDeviceSize size,
								    VkBufferUsageFlags usage)
{
	if (data == nullptr || size == 0) 
	{
		VWZ_LOG_ERROR("Buffer::createWithStaging: invalid parameters(data is null or size=0)");
		return false;
	}

	Buffer staging;
	if (!staging.create(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) 
	{
		VWZ_LOG_ERROR("Buffer::createWithStaging: failed to create staging buffer");
		return false;
	}
	void* mapped = staging.map();
	if (mapped == nullptr) 
	{
		VWZ_LOG_ERROR("Buffer::createWithStaging: failed to map staging buffer");
		return false;
	}
	std::memcpy(mapped, data, static_cast<std::size_t>(size));
	staging.unmap();

	if (!create(device, physicalDevice, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) 
	{
		VWZ_LOG_ERROR("Buffer::createWithStaging: failed to create device-local buffer");
		return false;
	}

	const bool copied = oneTimeSubmit(device, queue, pool,
	[&](VkCommandBuffer cmd) 
	{
		const VkBufferCopy region{ 0, 0, size };
		vkCmdCopyBuffer(cmd, staging.handle(), m_buffer, 1, &region);
	});
	if (!copied) 
	{
		VWZ_LOG_ERROR("Buffer::createWithStaging: failed to submit copy");
		destroy();
		return false;
	}

	VWZ_LOG_INFO("Buffer uploaded: {} bytes(staging -> device-local)", size);
	return true;
}

void zvk::Buffer::destroy()
{
	if (m_mapped != nullptr) 
	{
		vkUnmapMemory(m_device, m_memory);
		m_mapped = nullptr;
	}
	if (m_buffer != VK_NULL_HANDLE) 
	{
		vkDestroyBuffer(m_device, m_buffer, nullptr);
		m_buffer = VK_NULL_HANDLE;
	}
	if (m_memory != VK_NULL_HANDLE) 
	{
		vkFreeMemory(m_device, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
	m_device = VK_NULL_HANDLE;
	m_size = 0;
}

void* zvk::Buffer::map()
{
	if (m_buffer == VK_NULL_HANDLE || m_mapped != nullptr) 
	{
		return m_mapped;
	}
	const VkResult result = vkMapMemory(m_device, m_memory, 0, m_size, 0, &m_mapped);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("Failed to map memory: {} ({})", resultName(result), static_cast<int>(result));
		m_mapped = nullptr;
	}
	return m_mapped;
}

void zvk::Buffer::unmap()
{
	if (m_mapped != nullptr) 
	{
		vkUnmapMemory(m_device, m_memory);
		m_mapped = nullptr;
	}
}