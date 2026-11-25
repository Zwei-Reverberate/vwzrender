#pragma once
#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <copymovepolicy.h>

namespace zvk
{
	std::string_view presentModeName(VkPresentModeKHR mode);
	std::string formatName(VkFormat format);
	std::string_view colorSpaceName(VkColorSpaceKHR colorSpace);
	std::string extentText(const VkSurfaceCapabilitiesKHR& capabilities);

	class Surface : private vwz::core::CopyMovePolicy<false>
	{
	public:
		Surface() = default;
		~Surface();

		Surface(const Surface&) = delete;
		Surface& operator=(const Surface&) = delete;

		bool create(VkInstance instance, HWND hwnd, HINSTANCE hinstance);
		void destroy();
		bool isValid() const { return m_surface != VK_NULL_HANDLE; }
		VkSurfaceKHR handle() const { return m_surface; }

		bool queryCapabilities(VkPhysicalDevice device, VkSurfaceCapabilitiesKHR& out) const;
		std::vector<VkSurfaceFormatKHR> queryFormats(VkPhysicalDevice device) const;
		std::vector<VkPresentModeKHR> queryPresentModes(VkPhysicalDevice device) const;
		bool supportsPresentation(VkPhysicalDevice device, std::uint32_t queueFamily) const;

	private:
		VkInstance m_instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	};
}