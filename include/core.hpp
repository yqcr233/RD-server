#ifndef CORE_HPP
#define CORE_HPP
#include <iostream>
#include <string>
#include <ctime>
#include <thread>
#include <queue>
#include <deque>
#include <condition_variable>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <winsock2.h> // windows socket api定义
#include <ws2tcpip.h> // windows socket api定义
#include <windows.h> // windows api定义，如句柄、线程函数
#include <winuser.h>
#include <memory>
#include <wrl/client.h>
#include <Windows.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wincodec.h>
#include <comdef.h>
#include <fstream>
#include <functional>
#include <typeinfo>
extern "C"{
    #include<libavcodec/avcodec.h>
    #include<libavformat/avformat.h>
    #include <libavdevice/avdevice.h>
    #include <libavfilter/avfilter.h>
    #include <libavutil/avutil.h>
    #include<libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
    #include <libavutil/hwcontext_d3d11va.h>
    #include <libswresample/swresample.h>
    #include <libavutil/time.h>
    #include <libavutil/dict.h>
    #include <d3d11.h>
    #include <dxgi.h>
    #include <dxgi1_2.h>
}
using namespace std;
#endif
