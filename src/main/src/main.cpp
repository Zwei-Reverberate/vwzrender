#include <iostream>
#include <demo/demo1.h>
#include <demo/demo2.h>
#include <demo/demo3.h>
#include <demo/demo4.h>
#include <demo/demo5.h>
#include <demo/demo6.h>
#include <log.h>

int main()
{
	vwz::log::init();

	demo::DemoBase<demo::Demo6> demo;
	demo.run();

	return 0;
}