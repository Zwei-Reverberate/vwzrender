#include <device.h>
#include <log.h>
#include <instance.h>
#include <algorithm>

constexpr std::uint32_t typeScore(VkPhysicalDeviceType type)
{
	switch (type)
	{
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 1000;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 500;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return 300;
	case VK_PHYSICAL_DEVICE_TYPE_CPU: return 100;
	default: break;
	}
	return 200;
}

bool supportsPresent(VkPhysicalDevice device, std::uint32_t queueFamily, VkSurfaceKHR surface)
{
	VkBool32 supported = VK_FALSE;
	if (vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamily, surface, &supported) != VK_SUCCESS)
	{
		return false;
	}
	return supported == VK_TRUE;
}

std::vector<VkExtensionProperties> enumerateDeviceExtensions(VkPhysicalDevice device)
{
	std::uint32_t count = 0;
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS || count == 0)
	{
		return {};
	}
	std::vector<VkExtensionProperties> extensions(count);
	const VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
	if (result != VK_SUCCESS && result != VK_INCOMPLETE)
	{
		return {};
	}
	extensions.resize(count);
	return extensions;
}

bool zvk::selectPhysicalDevice(VkSurfaceKHR surface, const std::vector<PhysicalDeviceInfo>& devices, DeviceSelection& out)
{
	out = {};
	if (surface == VK_NULL_HANDLE)
	{
		VWZ_LOG_ERROR("selectPhysicalDevice: surface is invalid(a present target surface is required)");
		return false;
	}
	if (devices.empty())
	{
		VWZ_LOG_ERROR("selectPhysicalDevice: candidate device list is empty");
		return false;
	}

	VWZ_LOG_INFO("Physical device selection({} candidate(s)):", devices.size());
	std::size_t bestIndex = devices.size();
	std::uint32_t bestScore = 0;

	for (std::size_t i = 0; i < devices.size(); ++i)
	{
		const PhysicalDeviceInfo& info = devices[i];
		const std::string_view typeName = deviceTypeName(info.properties.deviceType);

		// Hard filter A: "can draw" -- a queue family with graphics capability is required
		std::uint32_t graphicsFamily = DeviceSelection::kInvalidFamily;
		for (std::uint32_t f = 0; f < info.queueFamilies.size(); ++f)
		{
			if (info.queueFamilies[f].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphicsFamily = f;
				break;
			}
		}
		if (graphicsFamily == DeviceSelection::kInvalidFamily)
		{
			VWZ_LOG_INFO("  [{}] {} ({}): rejected -- no graphics queue family", i, info.properties.deviceName, typeName);
			continue;
		}

		// Hard filter B: "can display" -- a queue family that supports presenting to the target surface is required.
		std::uint32_t presentFamily = DeviceSelection::kInvalidFamily;
		for (std::uint32_t f = 0; f < info.queueFamilies.size(); ++f)
		{
			if (supportsPresent(info.handle, f, surface))
			{
				presentFamily = f;
				break;
			}
		}
		if (presentFamily == DeviceSelection::kInvalidFamily)
		{
			VWZ_LOG_INFO("  [{}] {} ({}): rejected -- cannot present to this surface", i, info.properties.deviceName, typeName);
			continue;
		}

		std::uint32_t transferFamily = graphicsFamily;
		for (std::uint32_t f = 0; f < info.queueFamilies.size(); ++f)
		{
			const VkQueueFlags flags = info.queueFamilies[f].queueFlags;
			if ((flags & VK_QUEUE_TRANSFER_BIT) != 0 && (flags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0)
			{
				transferFamily = f;
				break;
			}
		}

		const std::uint32_t score = typeScore(info.properties.deviceType);
		VWZ_LOG_INFO("  [{}] {} ({}): score {} (graphics family [{}], present family [{}])", i, info.properties.deviceName, typeName, score, graphicsFamily, presentFamily);

		if (score > bestScore)
		{
			bestScore = score;
			bestIndex = i;
			out.handle = info.handle;
			out.deviceName = info.properties.deviceName;
			out.deviceType = info.properties.deviceType;
			out.graphicsFamily = graphicsFamily;
			out.presentFamily = presentFamily;
			out.transferFamily = transferFamily;
		}
	}

	if (bestIndex >= devices.size())
	{
		VWZ_LOG_ERROR("No physical device meets the requirements(graphics queue family + presentation to the target surface)");
		return false;
	}

	out.score = bestScore;
	out.reason = std::format("{} has the highest score({});graphics and present queue families available", deviceTypeName(out.deviceType), out.score);
	VWZ_LOG_INFO("Selected [{}] {}:graphics family [{}] / present family [{}](reason: {})", bestIndex, out.deviceName, out.graphicsFamily, out.presentFamily, out.reason);
	VWZ_LOG_INFO("Transfer family: [{}]{}", out.transferFamily,
		out.transferFamily == out.graphicsFamily
		? "(no dedicated transfer family; shared with graphics -- uploads use the graphics family's transfer capability)"
		: "(dedicated transfer family)");

	return true;
}

zvk::Device::~Device()
{
	destroy();
}

void zvk::Device::destroy()
{
	if (m_device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
		VWZ_LOG_INFO("VkDevice destroyed");
	}
	m_graphicsQueue = VK_NULL_HANDLE;
	m_presentQueue = VK_NULL_HANDLE;
	m_transferQueue = VK_NULL_HANDLE;
}

bool zvk::Device::waitIdle() const
{
	if (!isValid())
	{
		return false;
	}
	const VkResult result = vkDeviceWaitIdle(m_device);
	if (result != VK_SUCCESS)
	{
		VWZ_LOG_ERROR("vkDeviceWaitIdle fail: {} ({})", resultName(result), static_cast<int>(result));
		return false;
	}
	return true;
}

bool zvk::Device::create(VkPhysicalDevice physicalDevice, const DeviceSelection& selection, const DeviceConfig& config)
{
	if (isValid())
	{
		VWZ_LOG_WARN("Device::create: Device has been created, ignoring duplicate calls.");
		return true;
	}
	if (physicalDevice == VK_NULL_HANDLE || !selection.isValid() || selection.graphicsFamily == DeviceSelection::kInvalidFamily)
	{
		VWZ_LOG_ERROR("Device::create: Invalid arguments(physical device or selection is missing)");
		return false;
	}

	std::vector<const char*> requestedExtensions;
	if (config.enableSwapchain)
	{
		requestedExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	for (const char* extra : config.extraExtensions)
	{
		requestedExtensions.push_back(extra);
	}

	const std::vector<VkExtensionProperties> availableExtensions = enumerateDeviceExtensions(physicalDevice);
	for (const char* name : requestedExtensions)
	{
		const bool found = std::any_of(availableExtensions.begin(), availableExtensions.end(), [&](const VkExtensionProperties& e) { return std::string_view(e.extensionName) == name; });
		if (!found)
		{
			VWZ_LOG_ERROR("The requested device extension is unavailable: {} (create aborted)", name);
			return false;
		}
	}

	const float queuePriority = 1.0f;
	std::vector<std::uint32_t> families{ selection.graphicsFamily };
	if (selection.presentFamily != selection.graphicsFamily)
	{
		families.push_back(selection.presentFamily);
	}

	if (selection.transferFamily != DeviceSelection::kInvalidFamily
		&& selection.transferFamily != selection.graphicsFamily
		&& selection.transferFamily != selection.presentFamily)
	{
		families.push_back(selection.transferFamily);
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(families.size());
	for (const std::uint32_t family : families)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = family;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	VkPhysicalDeviceVulkan13Features features13{};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	if (config.enableDynamicRendering)
	{
		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		const std::uint32_t deviceApi = deviceProperties.apiVersion;
		if (VK_API_VERSION_MAJOR(deviceApi) < 1 || VK_API_VERSION_MINOR(deviceApi) < 3)
		{
			VWZ_LOG_ERROR("Device API version {}.{} < 1.3: dynamic rendering is unavailable; create aborted",
				VK_API_VERSION_MAJOR(deviceApi), VK_API_VERSION_MINOR(deviceApi));
			return false;
		}
		VkPhysicalDeviceVulkan13Features available13{};
		available13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &available13;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
		if (available13.dynamicRendering != VK_TRUE)
		{
			VWZ_LOG_ERROR("Device does not support dynamicRendering; create aborted");
			return false;
		}
		features13.dynamicRendering = VK_TRUE;
	}

	VkPhysicalDeviceFeatures availableCore{};
	vkGetPhysicalDeviceFeatures(physicalDevice, &availableCore);
	VkPhysicalDeviceFeatures enabledCore{};
	if (config.enableSamplerAnisotropy)
	{
		if (availableCore.samplerAnisotropy == VK_TRUE)
		{
			enabledCore.samplerAnisotropy = VK_TRUE;
		}
		else
		{
			VWZ_LOG_WARN("Device does not support samplerAnisotropy; anisotropic sampling is skipped(falls back to isotropic)");
		}
	}

	VkPhysicalDeviceVulkan12Features features12{};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	bool features12Enabled = false;
	if (config.enableDescriptorIndexing)
	{
		VkPhysicalDeviceVulkan12Features available12{};
		available12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		VkPhysicalDeviceFeatures2 probe2{};
		probe2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		probe2.pNext = &available12;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &probe2);
		const bool allRequired =
			available12.descriptorIndexing == VK_TRUE
			&& available12.runtimeDescriptorArray == VK_TRUE
			&& available12.shaderSampledImageArrayNonUniformIndexing == VK_TRUE
			&& available12.descriptorBindingPartiallyBound == VK_TRUE
			&& available12.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE;
		if (allRequired)
		{
			features12.descriptorIndexing = VK_TRUE;
			features12.runtimeDescriptorArray = VK_TRUE;
			features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
			features12.descriptorBindingPartiallyBound = VK_TRUE;
			features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
			features12Enabled = true;
		}
		else
		{
			VWZ_LOG_WARN("Device feature: descriptorIndexing = OFF");
		}
	}

	if (config.enableDrawIndirectCount)
	{
		VkPhysicalDeviceVulkan12Features available12{};
		available12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		VkPhysicalDeviceFeatures2 probe2{};
		probe2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		probe2.pNext = &available12;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &probe2);
		if (available12.drawIndirectCount == VK_TRUE)
		{
			features12.drawIndirectCount = VK_TRUE;
			features12Enabled = true;
		}
		else
		{
			VWZ_LOG_WARN("Device feature: drawIndirectCount = OFF");
		}
	}

	if (config.enableTimelineSemaphore)
	{
		VkPhysicalDeviceVulkan12Features available12{};
		available12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		VkPhysicalDeviceFeatures2 probe2{};
		probe2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		probe2.pNext = &available12;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &probe2);
		if (available12.timelineSemaphore == VK_TRUE)
		{
			features12.timelineSemaphore = VK_TRUE;
			features12Enabled = true;
		}
		else
		{
			VWZ_LOG_WARN("Device feature: timelineSemaphore = OFF");
		}
	}

	VkPhysicalDeviceFeatures2 features2{};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.features = enabledCore;
	if (features12Enabled)
	{
		features2.pNext = &features12;
	}
	if (config.enableDynamicRendering)
	{
		if (features12Enabled)
		{
			features12.pNext = &features13;
		}
		else
		{
			features2.pNext = &features13;
		}
	}
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &features2;
	createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<std::uint32_t>(requestedExtensions.size());
	createInfo.ppEnabledExtensionNames = requestedExtensions.data();

	const VkResult createResult = vkCreateDevice(physicalDevice, &createInfo, nullptr, &m_device);
	if (createResult != VK_SUCCESS)
	{
		m_device = VK_NULL_HANDLE;
		VWZ_LOG_ERROR("vkCreateDevice fail: {} ({})", resultName(createResult), static_cast<int>(createResult));
		return false;
	}

	vkGetDeviceQueue(m_device, selection.graphicsFamily, 0, &m_graphicsQueue);
	if (selection.presentFamily == selection.graphicsFamily)
	{
		m_presentQueue = m_graphicsQueue;
		VWZ_LOG_INFO("Queue family: graphics [{}] + present [{}] share one family; both use queue 0", selection.graphicsFamily, selection.presentFamily);
	}
	else
	{
		vkGetDeviceQueue(m_device, selection.presentFamily, 0, &m_presentQueue);
		VWZ_LOG_INFO("Queue family: graphics [{}] and present [{}] are different families; each uses queue 0", selection.graphicsFamily, selection.presentFamily);
	}

	{
		const std::uint32_t transferFamily =
			selection.transferFamily == DeviceSelection::kInvalidFamily
			? selection.graphicsFamily : selection.transferFamily;
		if (transferFamily != selection.graphicsFamily)
		{
			vkGetDeviceQueue(m_device, transferFamily, 0, &m_transferQueue);
			VWZ_LOG_INFO("Queue family: transfer [{}] is a dedicated family; queue 0 retrieved separately", transferFamily);
		}
		else
		{
			m_transferQueue = m_graphicsQueue;
			VWZ_LOG_INFO("Queue family: transfer shares graphics [{}](no dedicated transfer family; uploads use the graphics family's transfer capability)",
				selection.graphicsFamily);
		}
	}

	VWZ_LOG_INFO("VkDevice created; queues retrieved, device extension(s): {}", requestedExtensions.size());
	for (const char* name : requestedExtensions)
	{
		VWZ_LOG_INFO("  - device extension: {}", name);
	}
	if (config.enableDynamicRendering)
	{
		VWZ_LOG_INFO("Device feature: dynamicRendering = ON (Vulkan 1.3 core)");
	}
	if (features12.descriptorIndexing == VK_TRUE)
	{
		VWZ_LOG_INFO("Device feature: descriptorIndexing = ON (Vulkan 1.2;runtime arrays/non-uniform indexing/partially bound/updateAfterBind)");
	}
	if (features12.drawIndirectCount == VK_TRUE)
	{
		VWZ_LOG_INFO("Device feature: drawIndirectCount = ON (Vulkan 1.2;draw count comes from GPU counters)");
	}
	if (features12.timelineSemaphore == VK_TRUE)
	{
		VWZ_LOG_INFO("Device feature: timelineSemaphore = ON (Vulkan 1.2;upload batch completion counters)");
	}
	if (enabledCore.samplerAnisotropy == VK_TRUE)
	{
		VkPhysicalDeviceProperties enabledProperties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &enabledProperties);
		VWZ_LOG_INFO("Device feature: samplerAnisotropy = ON (hardware limit x{:.0f})", enabledProperties.limits.maxSamplerAnisotropy);
	}
	return true;
}