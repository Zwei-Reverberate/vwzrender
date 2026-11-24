#pragma once
#include <demo/demobase.h>

namespace demo
{
	class Demo5 : public DemoBase<Demo5>
	{
	private:
		void runVkDevice();

	public:
		void runImpl();
	};
}