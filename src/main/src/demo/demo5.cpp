#include <demo/demo5.h>
#include <demo/demo2.h>
#include <log.h>
#include <instance.h>
#include <physicaldevice.h>
#include <surface.h>
#include <device.h>
#include <zwindow.h>

void demo::Demo5::runVkDevice()
{
	VWZ_LOG_INFO("");
	VWZ_LOG_INFO("---- 设备与表面(--vk-device) ----");

	zvk::InstanceConfig config{};
	config.appName = "VulkanBase";
	config.appVersion = VK_MAKE_API_VERSION(0, 0, 5, 0);
	zvk::Instance instance;
	if (!instance.create(config))
	{
		VWZ_LOG_ERROR("VK-DEVICE SUMMARY: instance=FAILED");
		return;
	}

	zwin::Application app{ 0, nullptr };

	auto window = createDemoWindow("第 5 课：设备与队列");
	if (window == nullptr)
	{
		return;
	}

	const zwin::NativeWindow native = window->nativeWindow();
	zvk::Surface surface;
	if (!surface.create(instance.handle(), static_cast<HWND>(native.window), static_cast<HINSTANCE>(native.instance)))
	{
		VWZ_LOG_ERROR("VK-DEVICE SUMMARY: surface=FAILED");
		return;
	}

	const std::vector<zvk::PhysicalDeviceInfo> devices = zvk::enumeratePhysicalDevices(instance.handle());
	zvk::DeviceSelection selection;
	if (!zvk::selectPhysicalDevice(surface.handle(), devices, selection))
	{
		VWZ_LOG_ERROR("VK-DEVICE SUMMARY: selection=FAILED");
		return;
	}

	VkSurfaceCapabilitiesKHR capabilities{};
	if (surface.queryCapabilities(selection.handle, capabilities))
	{
		if (capabilities.maxImageCount == 0)
		{
			VWZ_LOG_INFO("表面能力: 图像数量 min={} max=不限", capabilities.minImageCount);
		}
		else
		{
			VWZ_LOG_INFO("表面能力: 图像数量 min={} max={}", capabilities.minImageCount, capabilities.maxImageCount);
		}
		VWZ_LOG_INFO("  当前尺寸: {} (min {}x{} / max {}x{})",
			zvk::extentText(capabilities),
			capabilities.minImageExtent.width, capabilities.minImageExtent.height,
			capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);
		VWZ_LOG_INFO("  图形族 [{}] 支持向该表面呈现: {}",
			selection.graphicsFamily,
			surface.supportsPresentation(selection.handle, selection.graphicsFamily) ? "是" : "否");
	}
	const std::vector<VkSurfaceFormatKHR> formats = surface.queryFormats(selection.handle);
	VWZ_LOG_INFO("表面格式({} 种):", formats.size());
	for (const VkSurfaceFormatKHR& format : formats)
	{
		VWZ_LOG_INFO("  - {} + {}", zvk::formatName(format.format), zvk::colorSpaceName(format.colorSpace));
	}
	const std::vector<VkPresentModeKHR> presentModes = surface.queryPresentModes(selection.handle);
	VWZ_LOG_INFO("呈现模式({} 种):", presentModes.size());
	for (const VkPresentModeKHR mode : presentModes)
	{
		VWZ_LOG_INFO("  - {}", zvk::presentModeName(mode));
	}

	zvk::DeviceConfig deviceConfig{};
	zvk::Device device;
	if (!device.create(selection.handle, selection, deviceConfig))
	{
		VWZ_LOG_ERROR("VK-DEVICE SUMMARY: device=FAILED");
		return;
	}

	VWZ_LOG_INFO("队列就绪: 图形={} 呈现={}", static_cast<const void*>(device.graphicsQueue()), static_cast<const void*>(device.presentQueue()));

	VWZ_LOG_INFO("窗口展示 3 秒(ESC 可提前退出)……");
	const auto showStart = std::chrono::steady_clock::now();
	while (window->processEvents())
	{
		if (std::chrono::steady_clock::now() - showStart > std::chrono::seconds(3))
		{
			VWZ_LOG_INFO("展示时间到,自动请求关闭");
			break;
		}
		::Sleep(16);
	}

	VWZ_LOG_INFO("校验层消息: {} 条错误 / {} 条警告", instance.errorCount(), instance.warningCount());
	VWZ_LOG_INFO("VK-DEVICE SUMMARY: surface=OK device=\"{}\" graphics_family={} present_family={} queues=OK swapchain={} validation={}",
		selection.deviceName, selection.graphicsFamily, selection.presentFamily,
		deviceConfig.enableSwapchain ? "ON" : "OFF",
		instance.validationEnabled() ? "ON" : "OFF");

	VWZ_LOG_INFO("---- 收尾:按依赖逆序销毁 ----");
	if (device.waitIdle())
	{
		VWZ_LOG_INFO("设备闲置确认(vkDeviceWaitIdle)");
	}
	device.destroy();
	surface.destroy();
	window.reset();
	instance.destroy();
	VWZ_LOG_INFO("全部对象已销毁");
}

void demo::Demo5::runImpl()
{
	runVkDevice();
}