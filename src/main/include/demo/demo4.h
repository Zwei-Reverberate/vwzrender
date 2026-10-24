#pragma once
#include <demo/demobase.h>

namespace demo
{
	class Demo4 : public DemoBase<Demo4>
	{
	private:
		void runVkInfo();

	public:
		void runImpl();
	};
}