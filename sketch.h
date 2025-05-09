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
#define MAX_DECODED_IMG_SIZE (4 * 1024 * 1024) // 4MB, adjust as needed for your images

// extern lv_obj_t *canvas; // canvas is static in sketch.cpp
// extern lv_color_t *cbuf; // cbuf is static in sketch.cpp

extern uint16_t* decoded_img_buffer; // Corrected to uint16_t*
extern int decoded_img_width;
extern int decoded_img_height;
extern size_t decoded_img_size; // Added this line
extern volatile bool new_image_available; 

// Add extern declarations for the dimensions received from WebSocket
extern int received_image_width;
extern int received_image_height;

void sketch_setup();  // to be called from setup
void sketch_loop();   // optional: if you want animation or interaction