#pragma once

#include <lvgl.h>

// Declare the IP address string as extern so sketch.cpp can access it
extern char ip_address_str[16];

// Declare extern variables for WebSocket control values
extern float ws_slider_value;
extern float ws_number_value;
extern char ws_text_value[1024]; // Increased for longer commands
extern int decoded_img_width;
extern int decoded_img_height;


void sketch_setup();  // to be called from setup
void sketch_loop();   // optional: if you want animation or interaction

// sketch.h
#define MAX_DECODED_IMG_SIZE (4* 1024 * 1024) // 4MB, adjust as needed for your images

extern uint8_t* decoded_img_buffer;
extern size_t decoded_img_size;
extern volatile bool new_image_available;
extern int decoded_img_width;
extern int decoded_img_height;

// Remove static definitions from here, they are in sketch.cpp
// static bool draw_r3_enabled = true;
// static bool draw_r4_enabled = false;
// static bool draw_r5_enabled = false; 

// static lv_obj_t *canvas;