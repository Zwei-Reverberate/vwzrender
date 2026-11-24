#include <demo/demo5.h>
#include <demo/demo2.h>
#include <log.h>
#include <instance.h>
#include <physicaldevice.h>
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
	}

	zwin::Application app{ 0, nullptr };

	auto window = createDemoWindow("第 5 课：设备与队列");
	if (window == nullptr)
	{
		return;
	}
}

void demo::Demo5::runImpl()
{
	runVkDevice();
}