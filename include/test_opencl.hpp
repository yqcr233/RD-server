#include <iostream>
#include <CL/opencl.hpp>

using namespace std;

// 设置opencl全局变量
extern cl::Platform platform;
extern cl::Device device;
extern cl::Context ctx;
extern cl::CommandQueue com_queue;

void init_cl();

void test();

void test_hello();