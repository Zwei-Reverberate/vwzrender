#include <demo/demo1.h>
#include <log.h>
#include <vector>

const char* demo::Demo1::buildConfigName()
{
#ifdef NDEBUG
	return "Release";
#else
	return "Debug";
#endif
}

std::string demo::Demo1::compilerName()
{
#if defined(_MSC_VER)
	return std::format("MSVC {}.{} (Visual Studio 2022)", _MSC_VER / 100, _MSC_VER % 100);
#else
	return "unknown compiler";
#endif
}

const char* demo::Demo1::architectureName()
{
#if defined(_M_X64)
	return "x64";
#elif defined(_M_ARM64)
	return "ARM64";
#else
	return "unknown";
#endif
}

std::string demo::Demo1::windowsVersion()
{
	using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);

	const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll == nullptr)
	{
		return "unknown";
	}

	const auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
		GetProcAddress(ntdll, "RtlGetVersion"));
	if (rtlGetVersion == nullptr)
	{
		return "unknown";
	}

	RTL_OSVERSIONINFOW info{};
	info.dwOSVersionInfoSize = sizeof(info);
	if (rtlGetVersion(&info) != 0)
	{
		return "unknown";
	}

	return std::format("Windows {}.{} (build {})",
		info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
}

void demo::Demo1::probeVulkanRuntime()
{
	VWZ_LOG_INFO("");
	VWZ_LOG_INFO("---- Vulkan 运行时探测 ----");

	std::uint32_t apiVersion = 0;
	VkResult result = vkEnumerateInstanceVersion(&apiVersion);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("vkEnumerateInstanceVersion 失败,错误码 {}", static_cast<int>(result));
		return;
	}

	VWZ_LOG_INFO("运行时 API 版本: {}.{}.{} (variant {})",
		VK_API_VERSION_MAJOR(apiVersion),
		VK_API_VERSION_MINOR(apiVersion),
		VK_API_VERSION_PATCH(apiVersion),
		VK_API_VERSION_VARIANT(apiVersion));

	std::uint32_t extensionCount = 0;
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("枚举实例扩展失败,错误码 {}", static_cast<int>(result));
		return;
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("获取实例扩展列表失败,错误码 {}", static_cast<int>(result));
		return;
	}

	VWZ_LOG_INFO("可用实例扩展: {} 个", extensionCount);
	for (const auto& extension : extensions) 
	{
		VWZ_LOG_INFO("  - {} (spec {})", extension.extensionName, extension.specVersion);
	}

	std::uint32_t layerCount = 0;
	result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("枚举实例层失败,错误码 {}", static_cast<int>(result));
		return;
	}

	std::vector<VkLayerProperties> layers(layerCount);
	result = vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
	if (result != VK_SUCCESS) 
	{
		VWZ_LOG_ERROR("获取实例层列表失败,错误码 {}", static_cast<int>(result));
		return;
	}

	VWZ_LOG_INFO("可用实例层: {} 个", layerCount);

	bool validationFound = false;
	for (const auto& layer : layers) 
	{
		const bool isValidation = (std::string_view(layer.layerName) == "VK_LAYER_KHRONOS_validation");
		validationFound = validationFound || isValidation;
		VWZ_LOG_INFO("  - {} (spec {}){}", layer.layerName, layer.specVersion, "");
	}

	if (validationFound) 
	{
		VWZ_LOG_INFO("VK_LAYER_KHRONOS_validation: 可用");
	}
	else 
	{
		VWZ_LOG_WARN("未找到 VK_LAYER_KHRONOS_validation,请确认安装了 Vulkan SDK 且层注册正常");
	}
}

void demo::Demo1::runImpl()
{
	probeVulkanRuntime();
}