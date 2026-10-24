#include <demo/demo4.h>
#include <log.h>
#include <instance.h>
#include <physicaldevice.h>

void demo::Demo4::runVkInfo()
{
	VWZ_LOG_INFO("");
	VWZ_LOG_INFO("---- Vulkan 初始化(--vk-info) ----");

	std::uint32_t runtimeVersion = 0;
	if (vkEnumerateInstanceVersion(&runtimeVersion) == VK_SUCCESS) 
	{
		VWZ_LOG_INFO("运行时 API 版本: {}.{}.{}",
		VK_API_VERSION_MAJOR(runtimeVersion),
		VK_API_VERSION_MINOR(runtimeVersion),
		VK_API_VERSION_PATCH(runtimeVersion));
	}

	zvk::InstanceConfig config{};
	config.appName = "VulkanBase";
	config.appVersion = VK_MAKE_API_VERSION(0, 0, 4, 0);

	zvk::Instance instance;
	if (!instance.create(config)) 
	{
		VWZ_LOG_ERROR("VK-INFO SUMMARY: instance=FAILED");
		return;
	}

	const std::vector<zvk::PhysicalDeviceInfo> devices = zvk::enumeratePhysicalDevices(instance.handle());
	VWZ_LOG_INFO("物理设备: {} 个", devices.size());
	for (std::size_t i = 0; i < devices.size(); ++i) 
	{
		const zvk::PhysicalDeviceInfo& info = devices[i];
		VWZ_LOG_INFO("  [{}] {}", i, info.properties.deviceName);
		VWZ_LOG_INFO("      类型: {} | 设备 API: {} | 驱动版本: {} (厂商自定义编码)",
			zvk::deviceTypeName(info.properties.deviceType),
			zvk::versionText(info.properties.apiVersion),
			info.properties.driverVersion);
		VWZ_LOG_INFO("      队列族: {} 个", info.queueFamilies.size());
		for (std::size_t f = 0; f < info.queueFamilies.size(); ++f) 
		{
			const VkQueueFamilyProperties& family = info.queueFamilies[f];
			VWZ_LOG_INFO("        - [{}] 队列数 {} | 能力: {} | 时间戳有效位数 {}",
				f, family.queueCount, zvk::queueFlagsText(family.queueFlags), family.timestampValidBits);
		}
	}

	VWZ_LOG_INFO("校验层消息: {} 条错误 / {} 条警告", instance.errorCount(), instance.warningCount());

	VWZ_LOG_INFO("VK-INFO SUMMARY: instance=OK validation={} devices={} surface={}",
		instance.validationEnabled() ? "ON" : "OFF",
		devices.size(),
		config.enableSurfaceExtensions ? "ON" : "OFF");

	instance.destroy();
}

void demo::Demo4::runImpl()
{
	runVkInfo();
}
