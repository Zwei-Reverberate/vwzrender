#pragma once

#include <Windows.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <atomic>
#include <cstdint>
#include <string_view>

namespace zvk
{
	std::string_view resultName(VkResult result);

	struct InstanceConfig 
	{
		std::string appName = "wvzrender";
		std::uint32_t appVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		std::string engineName = "vwzrender";
		std::uint32_t engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		std::uint32_t requestedApiVersion = VK_API_VERSION_1_3;

#ifdef NDEBUG
		bool enableValidation = false;
#else
		bool enableValidation = true;
#endif

		bool enableSurfaceExtensions = true;
		std::vector<const char*> extraExtensions;
	};

	class Instance
	{
	public:
		Instance() = default;

		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;

		bool isValid() const { return m_instance != VK_NULL_HANDLE; }
		VkInstance handle() const { return m_instance; }
		std::uint32_t apiVersion() const { return m_apiVersion; }
		bool validationEnabled() const { return m_validationEnabled; }
		std::uint32_t errorCount() const { return m_errorCount.load(); }
		std::uint32_t warningCount() const { return m_warningCount.load(); }

		bool create(const InstanceConfig& config);
		void destroy();

	private:
		std::vector<VkLayerProperties> enumerateInstanceLayers();
		std::vector<VkExtensionProperties> enumerateInstanceExtensions();
		bool isValidationLayerAvailable();

		static VKAPI_ATTR VkBool32 VKAPI_CALL handleDebugMessage
		(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* data,
			void* userData
		);
		static std::string messageTypeText(VkDebugUtilsMessageTypeFlagsEXT type);
		
	private:
		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		std::atomic<std::uint32_t> m_errorCount{ 0 };
		std::atomic<std::uint32_t> m_warningCount{ 0 };
		std::uint32_t m_apiVersion = 0;
		bool m_validationEnabled = false;
	};
}