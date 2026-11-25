#include <instance.h>
#include <surface.h>
#include <log.h>
#include <format>

std::string_view zvk::presentModeName(VkPresentModeKHR mode)
{
	switch (mode)
	{
	case VK_PRESENT_MODE_IMMEDIATE_KHR: return "IMMEDIATE(no vsync; may tear)";
	case VK_PRESENT_MODE_MAILBOX_KHR: return "MAILBOX(low latency, no tearing)";
	case VK_PRESENT_MODE_FIFO_KHR: return "FIFO(vsync; always supported)";
	case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO_RELAXED(vsync only after tearing)";
	case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR: return "SHARED_DEMAND_REFRESH(shared, on demand)";
	case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return "SHARED_CONTINUOUS_REFRESH(shared, continuous)";
	default: break;
	}
	return "UNKNOWN";
}

std::string zvk::formatName(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8_UNORM(linear)";
	case VK_FORMAT_B8G8R8A8_SRGB: return "B8G8R8A8_SRGB(sRGB encoded)";
	case VK_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8_UNORM(linear)";
	case VK_FORMAT_R8G8B8A8_SRGB: return "R8G8B8A8_SRGB(sRGB encoded)";
	case VK_FORMAT_R16G16B16A16_SFLOAT: return "R16G16B16A16_SFLOAT(half float/HDR)";
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "A2B10G10R10_UNORM(common for HDR10)";
	case VK_FORMAT_D32_SFLOAT: return "D32_SFLOAT(32-bit float depth)";
	case VK_FORMAT_D32_SFLOAT_S8_UINT: return "D32_SFLOAT_S8_UINT(depth+stencil)";
	case VK_FORMAT_D24_UNORM_S8_UINT: return "D24_UNORM_S8_UINT(depth+stencil)";
	default: break;
	}
	return std::format("VkFormat({})", static_cast<int>(format));
}

std::string_view zvk::colorSpaceName(VkColorSpaceKHR colorSpace)
{
	switch (colorSpace)
	{
	case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return "SRGB_NONLINEAR(standard sRGB)";
	case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT: return "DISPLAY_P3(wide gamut)";
	case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: return "EXTENDED_SRGB_LINEAR(linear HDR)";
	case VK_COLOR_SPACE_HDR10_ST2084_EXT: return "HDR10_ST2084(PQ)";
	default: break;
	}
	return "UNKNOWN";
}

std::string zvk::extentText(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width == 0xFFFFFFFFu)
	{
		return "decided by the application within min..max";
	}
	return std::format("{}x{}(decided by the surface)", capabilities.currentExtent.width, capabilities.currentExtent.height);
}

zvk::Surface::~Surface()
{
	destroy();
}

void zvk::Surface::destroy()
{
	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
		VWZ_LOG_INFO("Surface destroyed");
	}
	m_instance = VK_NULL_HANDLE;
}

bool zvk::Surface::queryCapabilities(VkPhysicalDevice device, VkSurfaceCapabilitiesKHR& out) const
{
	out = {};
	if (!isValid() || device == VK_NULL_HANDLE)
	{
		return false;
	}

	const VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &out);
	if (result != VK_SUCCESS)
	{
		VWZ_LOG_ERROR("vkGetPhysicalDeviceSurfaceCapabilitiesKHR fail: {} ({})", resultName(result), static_cast<int>(result));
		return false;
	}
	return true;
}

std::vector<VkSurfaceFormatKHR> zvk::Surface::queryFormats(VkPhysicalDevice device) const
{
	std::vector<VkSurfaceFormatKHR> formats;
	if (!isValid() || device == VK_NULL_HANDLE)
	{
		return formats;
	}

	std::uint32_t count = 0;
	VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &count, nullptr);
	if (result != VK_SUCCESS || count == 0)
	{
		VWZ_LOG_ERROR("vkGetPhysicalDeviceSurfaceFormatsKHR(count) fail: {} ({})", resultName(result), static_cast<int>(result));
		return formats;
	}
	formats.resize(count);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &count, formats.data());
	if (result != VK_SUCCESS && result != VK_INCOMPLETE)
	{
		VWZ_LOG_ERROR("vkGetPhysicalDeviceSurfaceFormatsKHR(fill) fail: {} ({})", resultName(result), static_cast<int>(result));
		return {};
	}
	formats.resize(count);

	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		VWZ_LOG_INFO("Surface formats: wildcard(device supports any format; application decides)");
	}
	return formats;
}

std::vector<VkPresentModeKHR> zvk::Surface::queryPresentModes(VkPhysicalDevice device) const
{
	std::vector<VkPresentModeKHR> modes;
	if (!isValid() || device == VK_NULL_HANDLE)
	{
		return modes;
	}

	std::uint32_t count = 0;
	VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &count, nullptr);
	if (result != VK_SUCCESS || count == 0)
	{
		VWZ_LOG_ERROR("vkGetPhysicalDeviceSurfacePresentModesKHR(count) fail: {} ({})", resultName(result), static_cast<int>(result));
		return modes;
	}
	modes.resize(count);
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &count, modes.data());
	if (result != VK_SUCCESS && result != VK_INCOMPLETE)
	{
		VWZ_LOG_ERROR("vkGetPhysicalDeviceSurfacePresentModesKHR(fill) fail: {} ({})", resultName(result), static_cast<int>(result));
		return {};
	}
	modes.resize(count);
	return modes;
}

bool zvk::Surface::supportsPresentation(VkPhysicalDevice device, std::uint32_t queueFamily) const
{
	if (!isValid() || device == VK_NULL_HANDLE)
	{
		return false;
	}

	VkBool32 supported = VK_FALSE;
	const VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamily, m_surface, &supported);
	if (result != VK_SUCCESS)
	{
		VWZ_LOG_ERROR("vkGetPhysicalDeviceSurfaceSupportKHR(family {}) fail: {} ({})", queueFamily, resultName(result), static_cast<int>(result));
		return false;
	}
	return supported == VK_TRUE;
}

bool zvk::Surface::create(VkInstance instance, HWND hwnd, HINSTANCE hinstance)
{
	if (isValid())
	{
		VWZ_LOG_WARN("Surface::create: Surface has been created, ignoring duplicate calls.");
		return true;
	}
	if (instance == VK_NULL_HANDLE || hwnd == nullptr || hinstance == nullptr)
	{
		VWZ_LOG_ERROR("Surface::create: Invalid arguments(instance={}, hwnd={}, hinstance={})",
			static_cast<const void*>(instance), static_cast<const void*>(hwnd),
			static_cast<const void*>(hinstance));
		return false;
	}
	m_instance = instance;

	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hinstance = hinstance;
	createInfo.hwnd = hwnd;

	const VkResult result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
	if (result != VK_SUCCESS)
	{
		m_surface = VK_NULL_HANDLE;
		m_instance = VK_NULL_HANDLE;
		VWZ_LOG_ERROR("vkCreateWin32SurfaceKHR fail: {} ({})", resultName(result), static_cast<int>(result));
		return false;
	}

	VWZ_LOG_INFO("Surface created: HWND=0x{:X} HINSTANCE=0x{:X}", reinterpret_cast<std::uintptr_t>(hwnd), reinterpret_cast<std::uintptr_t>(hinstance));
	return true;
}