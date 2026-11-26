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

void demo::Demo4::runVkErrorTest()
{
	VWZ_LOG_INFO("");
	VWZ_LOG_INFO("---- 校验层拦截演示(--vk-error-test) ----");

	bool extRejected = false;
	{
		VWZ_LOG_INFO("第一枪:申请一个不存在的实例扩展");
		zvk::InstanceConfig config{};
		config.enableValidation = true;
		config.enableSurfaceExtensions = false;
		static constexpr const char* kMissingExtension = "VK_KHR_this_does_not_exist";
		config.extraExtensions = { kMissingExtension };

		zvk::Instance instance;
		extRejected = !instance.create(config);
		VWZ_LOG_INFO("  创建被拒绝: {}", extRejected ? "是 ✓" : "否 ✗(居然成功了)");
		instance.destroy();
	}

	std::uint32_t vuidCaptured = 0;
	bool instanceOk = false;
	{
		VWZ_LOG_INFO("第二枪:创建 Instance 后故意传非法参数");
		zvk::InstanceConfig config{};
		config.enableValidation = true;
		config.enableSurfaceExtensions = false;
		zvk::Instance instance;
		instanceOk = instance.create(config);

		if (instanceOk) 
		{
			const std::uint32_t errorsBefore = instance.errorCount();
			// pPhysicalDeviceCount 是必需指针:传 nullptr 违反
			// VUID-vkEnumeratePhysicalDevices-pPhysicalDeviceCount-parameter
			const VkResult result = vkEnumeratePhysicalDevices(instance.handle(), nullptr, nullptr);
			VWZ_LOG_INFO("  调用返回: {} ({})",
			zvk::resultName(result), static_cast<int>(result));
			vuidCaptured = instance.errorCount() - errorsBefore;
			VWZ_LOG_INFO("  校验层新增错误: {} 条", vuidCaptured);
		}
		instance.destroy();
	}

	VWZ_LOG_INFO("VK-ERROR-TEST SUMMARY: ext_rejected={} vuid_captured={}", extRejected ? 1 : 0, vuidCaptured);
}

void demo::Demo4::runImpl()
{
	runVkInfo();
	runVkErrorTest();
}
