#line 1 "/Users/liubo/Documents/Arduino/examples/lvgl_sketch_web/sketch.h"
#pragma once

#include <lvgl.h>

// Declare the IP address string as extern so sketch.cpp can access it
extern char ip_address_str[16];

// Declare extern variables for WebSocket control values
extern float ws_slider_value;
extern float ws_number_value;
extern char ws_text_value[64]; // Adjust size as needed

void sketch_setup();  // to be called from setup
void sketch_loop();   // optional: if you want animation or interaction