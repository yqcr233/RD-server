#include "control.hpp"

KeyboardEvent unserialize(char *buffer){
    KeyboardEvent keyevent;
    memcpy(&keyevent, buffer, sizeof(KeyboardEvent));
    return keyevent;
}

MouseButtonEvent unserializeMouseButton(char* buffer){
    MouseButtonEvent mousebutton;
    memcpy(&mousebutton, buffer, sizeof(mousebutton));
    return mousebutton;
}

MouseWheelEvent unserializeMouseWheel(char* buffer){
    MouseWheelEvent mousewheel;
    memcpy(&mousewheel, buffer, sizeof(mousewheel));
    return mousewheel;
}

MouseMoveEvent unserializeMouseMove(char* buffer){
    MouseMoveEvent mousemove;
    memcpy(&mousemove, buffer, sizeof(mousemove));
    return mousemove;
}