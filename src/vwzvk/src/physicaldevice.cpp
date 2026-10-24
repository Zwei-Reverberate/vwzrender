#include <physicaldevice.h>
#include <instance.h>
#include <log.h>

std::vector<zvk::PhysicalDeviceInfo> zvk::enumeratePhysicalDevices(VkInstance instance)
{
	std::vector<PhysicalDeviceInfo> devices;

	std::uint32_t count = 0;
	VkResult result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("vkEnumeratePhysicalDevices (count) failed: {} ({})", resultName(result), static_cast<int>(result));
		return devices;
	}
	if (count == 0) 
	{
		VWZ_LOG_ERROR("No physical devices found (no usable Vulkan implementation on the system)");
		return devices;
	}

	std::vector<VkPhysicalDevice> handles(count);
	result = vkEnumeratePhysicalDevices(instance, &count, handles.data());
	if (result != VK_SUCCESS && result != VK_INCOMPLETE) 
	{
		VWZ_LOG_ERROR("vkEnumeratePhysicalDevices (fill) failed: {} ({})", resultName(result), static_cast<int>(result));
		return devices;
	}
	handles.resize(count);

	devices.reserve(handles.size());
	for (const VkPhysicalDevice handle : handles) 
	{
		PhysicalDeviceInfo info;
		info.handle = handle;

		vkGetPhysicalDeviceProperties(handle, &info.properties);

		std::uint32_t familyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(handle, &familyCount, nullptr);
		info.queueFamilies.resize(familyCount);
		if (familyCount > 0) 
		{
			vkGetPhysicalDeviceQueueFamilyProperties(handle, &familyCount, info.queueFamilies.data());
			info.queueFamilies.resize(familyCount);
		}

		devices.push_back(std::move(info));
	}
	return devices;
}

std::string_view zvk::deviceTypeName(VkPhysicalDeviceType type)
{
	switch (type) 
	{
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "Discrete GPU";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "Virtual GPU";
	case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU software implementation";
	default: break;
	}
	return "Other";
}

std::string zvk::queueFlagsText(VkQueueFlags flags)
{
	std::string text;
	const auto append = [&text](const char* part) 
	{
		if (!text.empty()) 
		{
			text += '|';
		}
		text += part;
	};
	if (flags & VK_QUEUE_GRAPHICS_BIT) 
	{
		append("graphics");
	}
	if (flags & VK_QUEUE_COMPUTE_BIT) 
	{
		append("compute");
	}
	if (flags & VK_QUEUE_TRANSFER_BIT) 
	{
		append("transfer");
	}
	if (flags & VK_QUEUE_SPARSE_BINDING_BIT) 
	{
		append("sparse binding");
	}
	if (flags & VK_QUEUE_PROTECTED_BIT) 
	{
		append("protected");
	}
	if (text.empty()) 
	{
		text = "none";
	}
	return text;
}

std::string zvk::versionText(std::uint32_t version)
{
	return std::format("{}.{}.{}",
	VK_API_VERSION_MAJOR(version),
	VK_API_VERSION_MINOR(version),
	VK_API_VERSION_PATCH(version));
}