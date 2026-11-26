#pragma once
#include <vulkan/vulkan.h>
#include <physicaldevice.h>
#include <cstdint>
#include <string>
#include <vector>
#include <copymovepolicy.h>

namespace zvk
{
	struct DeviceSelection
	{
		static constexpr std::uint32_t kInvalidFamily = 0xFFFFFFFFu;
		VkPhysicalDevice handle = VK_NULL_HANDLE;
		std::string deviceName;
		VkPhysicalDeviceType deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
		std::uint32_t score = 0;
		std::string reason;
		std::uint32_t graphicsFamily = kInvalidFamily;
		std::uint32_t presentFamily = kInvalidFamily;
		std::uint32_t transferFamily = kInvalidFamily;
		bool isValid() const { return handle != VK_NULL_HANDLE; }
	};

	bool selectPhysicalDevice(VkSurfaceKHR surface, const std::vector<PhysicalDeviceInfo>& devices, DeviceSelection& out);

	struct DeviceConfig
	{
		bool enableSwapchain = true;
		bool enableDynamicRendering = true;
		bool enableSamplerAnisotropy = true;
		bool enableDescriptorIndexing = true;
		bool enableDrawIndirectCount = true;
		bool enableTimelineSemaphore = true;
		std::vector<const char*> extraExtensions;
	};

	class Device : private vwz::core::CopyMovePolicy<Device, false>
	{
	public:
		Device() = default;
		~Device();

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		bool create(VkPhysicalDevice physicalDevice, const DeviceSelection& selection, const DeviceConfig& config = {});
		void destroy();

		bool isValid() const { return m_device != VK_NULL_HANDLE; }
		VkDevice handle() const { return m_device; }
		VkQueue graphicsQueue() const { return m_graphicsQueue; }
		VkQueue presentQueue() const { return m_presentQueue; }
		VkQueue transferQueue() const { return m_transferQueue; }
		bool waitIdle() const;

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkQueue m_graphicsQueue = VK_NULL_HANDLE;
		VkQueue m_presentQueue = VK_NULL_HANDLE;
		VkQueue m_transferQueue = VK_NULL_HANDLE;
	};
}