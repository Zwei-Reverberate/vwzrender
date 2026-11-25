#pragma once
#include <demo/demobase.h>

namespace demo
{
	class Demo6 : public DemoBase<Demo6>
	{
	private:
		void runVkCommand();

	public:
		void runImpl();
	};
}