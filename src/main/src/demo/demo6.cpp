#include <demo/demo6.h>
#include <demo/demo2.h>
#include <command.h>
#include <buffer.h>
#include <log.h>
#include <instance.h>
#include <zwindow.h>
#include <surface.h>
#include <physicaldevice.h>
#include <device.h>

void demo::Demo6::runVkCommand()
{
	VWZ_LOG_INFO("");
	VWZ_LOG_INFO("---- 命令系统(--vk-cmd) ----");


	zvk::InstanceConfig config{};
	config.appName = "VulkanBase";
	config.appVersion = VK_MAKE_API_VERSION(0, 0, 6, 0);
	zvk::Instance instance;
	if (!instance.create(config)) 
	{
		VWZ_LOG_ERROR("VK-CMD SUMMARY: instance=FAILED");
		return;
	}

	zwin::Application app{ 0, nullptr };
	auto window = createDemoWindow("第 6 课：命令系统");
	if (window == nullptr)
	{
		return;
	}
	const zwin::NativeWindow native = window->nativeWindow();
	zvk::Surface surface;
	if (!surface.create(instance.handle(),
		static_cast<HWND>(native.window), static_cast<HINSTANCE>(native.instance))) 
	{
		VWZ_LOG_ERROR("VK-CMD SUMMARY: surface=FAILED");
		return;
	}
	VWZ_LOG_INFO("[2/5] 窗口就绪(未显示) + Surface 就位");

	
	const std::vector<zvk::PhysicalDeviceInfo> devices = zvk::enumeratePhysicalDevices(instance.handle());
	zvk::DeviceSelection selection;
	if (!zvk::selectPhysicalDevice(surface.handle(), devices, selection)) 
	{
		VWZ_LOG_ERROR("VK-CMD SUMMARY: selection=FAILED");
		return;
	}
	zvk::Device device;
	if (!device.create(selection.handle, selection)) 
	{
		VWZ_LOG_ERROR("VK-CMD SUMMARY: device=FAILED");
		return;
	}
	const VkQueue queue = device.graphicsQueue();
	zvk::CommandPool pool;
	if (!pool.create(device.handle(), selection.graphicsFamily)) 
	{
		VWZ_LOG_ERROR("VK-CMD SUMMARY: pool=FAILED");
		return;
	}
	VWZ_LOG_INFO("命令池就绪: 队列族 {} 旗标=RESET_COMMAND_BUFFER", selection.graphicsFamily);

	constexpr VkDeviceSize kFillBytes = 4096;
	constexpr std::uint32_t kPattern = 0xABABABABu;
	zvk::Buffer buffer;
	if (!buffer.create(device.handle(), selection.handle, kFillBytes,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) 
	{
		VWZ_LOG_ERROR("VK-CMD SUMMARY: buffer=FAILED");
		return;
	}

	{
		void* mapped = buffer.map();
		if (mapped == nullptr) 
		{
			VWZ_LOG_ERROR("VK-CMD SUMMARY: map=FAILED(预清零)");
			return;
		}
		std::memset(mapped, 0, static_cast<std::size_t>(kFillBytes));
		buffer.unmap();
	}
	VWZ_LOG_INFO("  CPU 预清零 {} 字节(读回非零即证明 GPU 写入)", kFillBytes);

	const bool submitted = zvk::oneTimeSubmit(device.handle(), queue, pool,
	[&buffer](VkCommandBuffer cmd) 
	{
		vkCmdFillBuffer(cmd, buffer.handle(), 0, VK_WHOLE_SIZE, kPattern);
	});
	if (!submitted) 
	{
		VWZ_LOG_ERROR("VK-CMD SUMMARY: submit=FAILED");
		return;
	}
	VWZ_LOG_INFO("  提交完成,围栏已触发(GPU 侧工作完成)");

	const std::size_t wordCount = static_cast<std::size_t>(kFillBytes / 4);
	std::size_t mismatches = 0;
	{
		const void* mapped = buffer.map();
		if (mapped == nullptr) 
		{
			VWZ_LOG_ERROR("VK-CMD SUMMARY: map=FAILED(回读)");
			return;
		}
		const auto* words = static_cast<const std::uint32_t*>(mapped);
		for (std::size_t i = 0; i < wordCount; ++i) 
		{
			if (words[i] != kPattern) 
			{
				++mismatches;
			}
		}
		buffer.unmap();
	}

	const bool pass = (mismatches == 0);
	if (pass) 
	{
		VWZ_LOG_INFO("  回读比对: {}/{} 字全部 = 0x{:08X} -> GPU 执行验证 PASS", wordCount, wordCount, kPattern);
	}
	else 
	{
		VWZ_LOG_ERROR("  回读比对: {} 个字不匹配 -> GPU 执行验证 FAIL", mismatches);
	}

	VWZ_LOG_INFO("校验层消息: {} 条错误 / {} 条警告", instance.errorCount(), instance.warningCount());
	VWZ_LOG_INFO("VK-CMD SUMMARY: pool=OK submit=OK fill_bytes={} pattern=0x{:08X} readback={} validation={}",
			     kFillBytes, kPattern,
		         pass ? "PASS" : "FAIL", instance.validationEnabled() ? "ON" : "OFF");

	VWZ_LOG_INFO("---- 收尾:按依赖逆序销毁 ----");
	buffer.destroy();   // 1) 缓冲:依赖设备
	pool.destroy();     // 2) 命令池:依赖设备
	device.destroy();   // 3) 逻辑设备:依赖表面与实例
	surface.destroy();  // 4) 表面:依赖实例
	window.reset();     // 5) 窗口(延迟销毁语义)
	instance.destroy(); // 6) 会话:最后
	VWZ_LOG_INFO("全部对象已销毁");
}

void demo::Demo6::runImpl()
{
	runVkCommand();
}
