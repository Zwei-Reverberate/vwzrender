#pragma once
#include <demo/demobase.h>
#include <memory>
#include <zwindow.h>

namespace demo
{
	std::unique_ptr<zwin::ZWindow> createDemoWindow(const char* title);
	const char* mouseButtonName(zwin::MouseButton button);

	class Demo2 : public DemoBase<Demo2>
	{
	public:
		void runImpl();
	};
}