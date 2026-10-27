#pragma once
namespace app {
void setup();
void review_configure(int component,int state,int dark,int size);
void draw();
void set_touch(bool enabled);
void safe_area(float top,float right,float bottom,float left);
void keyboard_height(float height);
void request_text_focus();
void zoom(float ratio);
}
