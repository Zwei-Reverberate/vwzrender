#include <instance.h>
#include <log.h>
#include <algorithm>

std::string_view zvk::resultName(VkResult result)
{
	switch (result) 
	{
	case VK_SUCCESS: return "VK_SUCCESS";
	case VK_NOT_READY: return "VK_NOT_READY";
	case VK_TIMEOUT: return "VK_TIMEOUT";
	case VK_INCOMPLETE: return "VK_INCOMPLETE";
	case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
	case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
	case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
	default: break;
	}
	return "VK_ERROR_<Other>";
}

bool zvk::Instance::create(const InstanceConfig& config)
{
	if (isValid()) 
	{
		VWZ_LOG_WARN("Instance has been created, ignoring duplicate calls.");
		return true;
	}

	std::uint32_t runtimeVersion = VK_API_VERSION_1_0;
	if (vkEnumerateInstanceVersion(&runtimeVersion) != VK_SUCCESS) 
	{
		runtimeVersion = VK_API_VERSION_1_0;
	}
	m_apiVersion = std::min(runtimeVersion, config.requestedApiVersion);
	VWZ_LOG_INFO("API version negotiation: runtime {}.{}.{} / request {}.{}.{} -> using {}.{}.{}",
	VK_API_VERSION_MAJOR(runtimeVersion), VK_API_VERSION_MINOR(runtimeVersion), VK_API_VERSION_PATCH(runtimeVersion),
	VK_API_VERSION_MAJOR(config.requestedApiVersion), VK_API_VERSION_MINOR(config.requestedApiVersion), VK_API_VERSION_PATCH(config.requestedApiVersion),
	VK_API_VERSION_MAJOR(m_apiVersion), VK_API_VERSION_MINOR(m_apiVersion), VK_API_VERSION_PATCH(m_apiVersion));


	m_validationEnabled = config.enableValidation && isValidationLayerAvailable();
	if (config.enableValidation && !m_validationEnabled) 
	{
		VWZ_LOG_WARN("Validation layer VK_LAYER_KHRONOS_validation is unavailable; falling back to running without validation.");
	}
	std::vector<const char*> enabledLayers;
	if (m_validationEnabled) 
	{
		enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
	}


	std::vector<const char*> enabledExtensions;
	if (m_validationEnabled) 
	{
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	if (config.enableSurfaceExtensions) 
	{
		enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);      
		enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	}
	for (const char* extra : config.extraExtensions) 
	{
		enabledExtensions.push_back(extra);
	}


	const std::vector<VkExtensionProperties> availableExtensions = enumerateInstanceExtensions();
	for (const char* name : enabledExtensions) 
	{
		const bool found = std::any_of(availableExtensions.begin(), availableExtensions.end(),
			[&](const VkExtensionProperties& e) { return std::string_view(e.extensionName) == name; });
		if (!found) 
		{
			VWZ_LOG_ERROR("The requested instance extension is unavailable: {} (create aborted)", name);
			return false;
		}
	}

	VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = &Instance::handleDebugMessage;
	debugInfo.pUserData = this;


	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = config.appName.c_str();
	appInfo.applicationVersion = config.appVersion;
	appInfo.pEngineName = config.engineName.c_str();
	appInfo.engineVersion = config.engineVersion;
	appInfo.apiVersion = m_apiVersion;


	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = m_validationEnabled ? &debugInfo : nullptr;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = static_cast<std::uint32_t>(enabledLayers.size());
	createInfo.ppEnabledLayerNames = enabledLayers.data();
	createInfo.enabledExtensionCount = static_cast<std::uint32_t>(enabledExtensions.size());
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();

	const VkResult createResult = vkCreateInstance(&createInfo, nullptr, &m_instance);
	if (createResult != VK_SUCCESS) 
	{
		m_instance = VK_NULL_HANDLE;
		VWZ_LOG_ERROR("vkCreateInstance fail: {} ({})", resultName(createResult), static_cast<int>(createResult));
		return false;
	}

	if (m_validationEnabled) 
	{
		const auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
		if (createMessenger != nullptr) 
		{
			const VkResult messengerResult = createMessenger(m_instance, &debugInfo, nullptr, &m_debugMessenger);
			if (messengerResult != VK_SUCCESS) 
			{
				VWZ_LOG_WARN("Debug callback creation failed: {} — validation layer messages will only appear on stderr.", resultName(messengerResult));
			}
		}
		else 
		{
			VWZ_LOG_WARN("Failed to load vkCreateDebugUtilsMessengerEXT — validation layer messages will only appear on stderr.");
		}
	}

	VWZ_LOG_INFO("Instance created: {} layer(s) / {} extension(s).", enabledLayers.size(), enabledExtensions.size());
	for (const char* name : enabledLayers) 
	{
		VWZ_LOG_INFO("  - layer: {}", name);
	}
	for (const char* name : enabledExtensions) 
	{
		VWZ_LOG_INFO("  - extension: {}", name);
	}

	return true;
}

void zvk::Instance::destroy()
{
	if (m_debugMessenger != VK_NULL_HANDLE) 
	{
		const auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (destroyMessenger != nullptr) 
		{
			destroyMessenger(m_instance, m_debugMessenger, nullptr);
		}
		m_debugMessenger = VK_NULL_HANDLE;
	}

	if (m_instance != VK_NULL_HANDLE) 
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
	m_validationEnabled = false;
}


std::vector<VkLayerProperties>zvk::Instance::enumerateInstanceLayers()
{
	std::uint32_t count = 0;
	if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS || count == 0)
	{
		return {};
	}

	std::vector<VkLayerProperties> layers(count);
	const VkResult result = vkEnumerateInstanceLayerProperties(&count, layers.data());
	if (result != VK_SUCCESS && result != VK_INCOMPLETE)
	{
		return {};
	}
	layers.resize(count);
	return layers;
}

std::vector<VkExtensionProperties>zvk::Instance::enumerateInstanceExtensions()
{
	std::uint32_t count = 0;
	if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS || count == 0) 
	{
		return {};
	}
	std::vector<VkExtensionProperties> extensions(count);
	const VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
	if (result != VK_SUCCESS && result != VK_INCOMPLETE) 
	{
		return {};
	}
	extensions.resize(count);
	return extensions;
}

bool zvk::Instance::isValidationLayerAvailable()
{
	for (const VkLayerProperties& layer : enumerateInstanceLayers()) 
	{
		if (std::string_view(layer.layerName) == "VK_LAYER_KHRONOS_validation") 
		{
			return true;
		}
	}
	return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL zvk::Instance::handleDebugMessage
(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* data,
	void* userData
)
{
	Instance& self = *static_cast<Instance*>(userData);

	const char* messageId = (data != nullptr && data->pMessageIdName != nullptr) ? data->pMessageIdName : "-";
	const char* message = (data != nullptr && data->pMessage != nullptr) ? data->pMessage : "(empty message)";
	const std::string typeText = messageTypeText(type);

	if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) 
	{
		self.m_errorCount.fetch_add(1, std::memory_order_relaxed);
		VWZ_LOG_ERROR("[vk/{}] {}: {}", typeText, messageId, message);
	}
	else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) 
	{
		self.m_warningCount.fetch_add(1, std::memory_order_relaxed);
		VWZ_LOG_WARN("[vk/{}] {}: {}", typeText, messageId, message);
	}
	else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) 
	{
		VWZ_LOG_INFO("[vk/{}] {}: {}", typeText, messageId, message);
	}
	else 
	{
		VWZ_LOG_DEBUG("[vk/{}] {}: {}", typeText, messageId, message);
	}

	return VK_FALSE;
}

std::string zvk::Instance::messageTypeText(VkDebugUtilsMessageTypeFlagsEXT type)
{
	std::string text;
	const auto append = [&text](const char* part) 
	{
		if (!text.empty()) 
		{
			text += '+';
		}
		text += part;
	};

	if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) 
	{
		append("general");
	}
	if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) 
	{
		append("validation");
	}
	if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) 
	{
		append("performance");
	}
	if (text.empty()) 
	{
		text = "unknown";
	}

	return text;
}