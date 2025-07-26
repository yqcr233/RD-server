#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <cstdint>
#include <iostream>

using namespace std;

extern int width;
extern int height;

// 通用事件头
struct InputEventHeader {
    uint8_t event_type;      // 事件类型标识
    uint16_t data_size;      // 附加数据大小
    uint32_t timestamp;      // 时间戳
};

// 键盘事件
struct KeyboardEvent : public InputEventHeader{
    uint8_t state;           // 状态 (0=释放, 1=按下)
    uint16_t scancode;       // 物理扫描码
    uint16_t modifiers;       // 修饰键状态
};

// 鼠标按键事件
struct MouseButtonEvent : public InputEventHeader{
    uint8_t button; // 按钮标识（例如：左键=0x01，右键=0x03）
    uint8_t down;   // 按下为1，释放为0
};

// 鼠标移动事件
struct MouseMoveEvent : public InputEventHeader{
    int32_t x;      // 鼠标的X坐标
    int32_t y;      // 鼠标的Y坐标
    int32_t width;
    int32_t height;
};

// 鼠标滚轮事件
struct MouseWheelEvent : public InputEventHeader{
    int32_t scroll_x; // 水平滚动（通常为0，除非支持横向滚动）
    int32_t scroll_y; // 垂直滚动
};

// 事件类型定义
enum EventTypes {
    MOUSE_MOVE_ABS = 0x01,   // 绝对坐标移动
    MOUSE_MOVE_REL = 0x02,   // 相对坐标移动
    MOUSE_BUTTON   = 0x03,   // 鼠标按键
    MOUSE_WHEEL    = 0x04,   // 鼠标滚轮
    KEYBOARD_EVENT = 0x05,   // 键盘事件
};

KeyboardEvent unserialize(char *buffer);

MouseButtonEvent unserializeMouseButton(char* buffer);

MouseWheelEvent unserializeMouseWheel(char* buffer);

MouseMoveEvent unserializeMouseMove(char* buffer);

#endif