#pragma once

#include <string>
#include <format>
#include <Windows.h>
#include <vulkan/vulkan.h>
#include <demo/demobase.h>

namespace demo
{
	class Demo1 : public DemoBase<Demo1>
	{
	private:
		const char* buildConfigName();
		std::string compilerName();
		const char* architectureName();
		std::string windowsVersion();
		void probeVulkanRuntime();

	public:
		void runImpl();
	};
}