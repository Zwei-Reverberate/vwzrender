#pragma once

namespace demo
{
	template<typename T>
	class DemoBase
	{
	public:
		void run() 
		{
			static_cast<T*>(this)->runImpl();
		}
	};
}