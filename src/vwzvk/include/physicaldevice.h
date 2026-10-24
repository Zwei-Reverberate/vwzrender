#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace zvk
{
	struct PhysicalDeviceInfo 
	{
		VkPhysicalDevice handle = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties properties{};
		std::vector<VkQueueFamilyProperties> queueFamilies;
	};

	std::vector<PhysicalDeviceInfo> enumeratePhysicalDevices(VkInstance instance);

	std::string_view deviceTypeName(VkPhysicalDeviceType type);

	std::string queueFlagsText(VkQueueFlags flags);

	std::string versionText(std::uint32_t version);
}