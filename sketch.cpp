
#include "sketch.h"
#include <Arduino.h>
#include <lvgl.h>
#include "esp_heap_caps.h"
#include "base64_utils.h"

#define CANVAS_WIDTH 480
#define CANVAS_HEIGHT 480
#define UPDATE_PERIOD 100 // milliseconds
#define LVGL_TICK_PERIOD 5

// Define extern variables declared in sketch.h
extern char ip_address_str[16];
extern float ws_slider_value;
extern float ws_number_value;
extern char ws_text_value[1024];

// Drawing algorithm toggles
static bool draw_r0_enabled = true;  // image background
static bool draw_r1_enabled = true;
static bool draw_r2_enabled = false;
static bool draw_r3_enabled = true;
static bool draw_r4_enabled = false;

static lv_obj_t *canvas;
static lv_color_t *cbuf = nullptr;


static lv_obj_t* img_widget = nullptr;
int decoded_img_width = 0;
int decoded_img_height = 0;

void check_image_update() {
    // r0: image background
    if (draw_r0_enabled && new_image_available && decoded_img_buffer != nullptr && decoded_img_size > 0) {
        // --- Assume JPEG decoder provides width/height and RGB565 data ---
        extern int decoded_img_width;
        extern int decoded_img_height;
        if (canvas && cbuf && decoded_img_width > 0 && decoded_img_height > 0) {
            // Draw image into the canvas buffer as the background (before generative art)
            // This will be visible underneath all generative art overlays
            lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);
            int src_w = decoded_img_width;
            int src_h = decoded_img_height;
            int dst_w = CANVAS_WIDTH;
            int dst_h = CANVAS_HEIGHT;
            float scale = fminf((float)dst_w / src_w, (float)dst_h / src_h);
            int draw_w = (int)(src_w * scale);
            int draw_h = (int)(src_h * scale);
            int x_off = (dst_w - draw_w) / 2;
            int y_off = (dst_h - draw_h) / 2;
            uint16_t* src = (uint16_t*)decoded_img_buffer;
            for (int y = 0; y < draw_h; ++y) {
                int src_y = (int)(y / scale);
                if (src_y >= src_h) src_y = src_h - 1;
                for (int x = 0; x < draw_w; ++x) {
                    int src_x = (int)(x / scale);
                    if (src_x >= src_w) src_x = src_w - 1;
                    uint16_t pixel = src[src_y * src_w + src_x];
                    int dst_x = x + x_off;
                    int dst_y = y + y_off;
                    if (dst_x >= 0 && dst_x < dst_w && dst_y >= 0 && dst_y < dst_h) {
                        cbuf[dst_y * dst_w + dst_x].full = pixel;
                    }
                }
            }
            // Do not call lv_obj_move_foreground/canvas layering here: canvas is always on top, image is drawn into canvas background
            lv_obj_invalidate(canvas);
        }
        new_image_available = false;
    }
}


static const lv_color_t palette[] = {
    lv_color_make(128, 0, 0),   // maroon
    lv_color_make(255, 215, 0), // gold
    lv_color_make(255, 0, 0),   // red
    lv_color_make(255, 33, 2),   // deep red

    lv_color_make(255, 165, 0),  // orange
    lv_color_make(255, 20, 147), // deeppink

    lv_color_make(255, 192, 203), // pink
    lv_color_make(250, 235, 215), // antiquewhite
    lv_color_make(0, 0, 0),       // black
    lv_color_make(0, 100, 0),     // hunter green
    lv_color_make(0, 128, 0),     // green

    lv_color_make(0, 0, 128),   // navy
    lv_color_make(0, 0, 255),   // blue
    lv_color_make(0, 191, 255), // deepskyblue
};

// palette = ["red","gold","pink",color(255,33,2)] //p5code

static const uint8_t palette_size = sizeof(palette) / sizeof(palette[0]);

// Timer callback to delete the IP label
static void ip_label_delete_timer_cb(lv_timer_t * timer) {
    Serial.println("-> ip_label_delete_timer_cb called"); // DEBUG
    lv_obj_t * label = (lv_obj_t *)timer->user_data;
    if(label) {
        Serial.println("   Deleting IP label..."); // DEBUG
        lv_obj_del(label);
    } else {
        Serial.println("   Label object was NULL in timer callback."); // DEBUG
    }
}


float frand(float a, float b)
{
  return a + ((float)random(0, 10000) / 10000.0f) * (b - a);
}

// Example: Use slider value to control line width range in draw_r1
static String lastTextValue = ""; // Store the last printed text value

static void draw_r1()
{
    // Map slider (0.0 to 1.0) to a line width range (e.g., 1 to 10)
    // Number controls thickness (1-20)
    int min_width = 1;
    int max_width = 20;
    int line_width = min_width + (int)(ws_number_value * (max_width - min_width));
    line_width = constrain(line_width, min_width, max_width); // Ensure width is within bounds

    // Slider controls opacity (0.0-1.0 mapped to LVGL 0-255)
    // If opacity is very low, don't draw the line at all (prevents black artifacts)
    uint8_t min_opa = 10; // allow almost transparent
    uint8_t max_opa = 255;
    uint8_t line_opa = min_opa + (uint8_t)(ws_slider_value * (max_opa - min_opa));
    if (ws_slider_value < 0.05f) return; // Don't draw if opacity is near zero

    // Use number to influence position range (optional, can keep as before)
    float center_offset_factor = 0.2f; // fixed, or could be another control
    float x = 0.5f, y = 0.5f; // Center
    float range_w = 0.8f * (1.0f - center_offset_factor); // Smaller range if number is higher
    float range_h = 0.8f * (1.0f - center_offset_factor);

    int x1 = frand(x - range_w / 2.0f, x + range_w / 2.0f) * CANVAS_WIDTH;
    int y1 = frand(y - range_h / 2.0f, y + range_h / 2.0f) * CANVAS_HEIGHT;
    int x2 = frand(x - range_w / 2.0f, x + range_w / 2.0f) * CANVAS_WIDTH;
    int y2 = frand(y - range_h / 2.0f, y + range_h / 2.0f) * CANVAS_HEIGHT;

    // Create an array of points for the line
    lv_point_t line_points[] = { {x1, y1}, {x2, y2} };

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = palette[random(0, palette_size)];
    line_dsc.width = line_width; // Use the controlled width
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;
    line_dsc.opa = line_opa;

    // Call the function with the points array and point count (2)
    lv_canvas_draw_line(canvas, line_points, 2, &line_dsc);

    // Example: Print text value if it's not the default and has changed
    if (strcmp(ws_text_value, "default") != 0) {
        String currentTextValue = String(ws_text_value);
        if (currentTextValue != lastTextValue) {
            Serial.printf("[Sketch] Current text: %s\n", ws_text_value);
            lastTextValue = currentTextValue; // Update the last printed value
        }
    }
}

// Draw randomly oriented, colored triangles with opacity and size control
void draw_r2() {
    // Triangle size proportional to ws_number_value (1-20 mapped to pixels)
    float min_size = 10.0f;
    float max_size = 200.0f;
    float tri_size = min_size + (ws_number_value - 1.0f) * (max_size - min_size) / 19.0f;
    tri_size = fmaxf(min_size, fminf(tri_size, max_size));
    // Don't draw if opacity is near zero
    if (ws_slider_value < 0.05f) return;

    // Center of triangle
    float cx = frand(0.1f, 0.9f) * CANVAS_WIDTH;
    float cy = frand(0.1f, 0.9f) * CANVAS_HEIGHT;

    // Random orientation
    float angle = frand(0, 2 * 3.1415926f);

    // Vertices of equilateral triangle
    lv_point_t pts[3];
    for (int i = 0; i < 3; ++i) {
        float a = angle + i * 2.0f * 3.1415926f / 3.0f;
        pts[i].x = (int)(cx + tri_size * cosf(a));
        pts[i].y = (int)(cy + tri_size * sinf(a));
    }

    // Color and opacity
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = palette[random(0, palette_size)];
    dsc.bg_opa = (lv_opa_t)(10 + ws_slider_value * (255 - 10));
    dsc.border_opa = LV_OPA_TRANSP;
    dsc.radius = 0;

    // Draw the triangle (filled polygon)
    lv_canvas_draw_polygon(canvas, pts, 3, &dsc);
}

// Example: Use number value to control arc width in draw_r3
static void draw_r3()
{
  lv_draw_rect_dsc_t fill_dsc;
  lv_draw_rect_dsc_init(&fill_dsc);
  fill_dsc.bg_color = palette[random(0, palette_size)];
  fill_dsc.radius = LV_RADIUS_CIRCLE;

  int cx = CANVAS_WIDTH / 2;
  int cy = CANVAS_HEIGHT / 2;
  int r = random(10, CANVAS_WIDTH / 3);

  // Number controls thickness (1-20)
  int min_arc_width = 1;
  int max_arc_width = 20;
  int arc_width = min_arc_width + (int)(ws_number_value * (max_arc_width - min_arc_width));
  arc_width = constrain(arc_width, min_arc_width, max_arc_width);

  // Slider controls opacity (0.0-1.0 mapped to LVGL 0-255)
  uint8_t min_opa = 10;
  uint8_t max_opa = 255;
  uint8_t arc_opa = min_opa + (uint8_t)(ws_slider_value * (max_opa - min_opa));
  if (ws_slider_value < 0.05f) return; // Don't draw if opacity is near zero

  // Draw fill arc (optional)
  // lv_canvas_draw_arc(canvas, cx, cy, r, 0, 360, &fill_dsc);

  lv_draw_arc_dsc_t dsc;
  lv_draw_arc_dsc_init(&dsc);
  dsc.color = palette[random(0, palette_size)];
  dsc.width = arc_width; // Use controlled width
  dsc.opa = arc_opa;

  int start_angle = random(0, 360);
  int end_angle = start_angle + random(30, 180); // Draw partial arcs

  lv_canvas_draw_arc(canvas, cx, cy, r, start_angle, end_angle, &dsc);
}

static void draw_r4()
{

  int x = random(0, CANVAS_WIDTH);
  int y = random(0, CANVAS_HEIGHT);

  lv_draw_arc_dsc_t dsc;
  lv_draw_arc_dsc_init(&dsc);
  dsc.color = palette[random(0, palette_size)];
  dsc.width = random(1, 5);
  lv_canvas_draw_arc(canvas, x, y, 20, 0, 360, &dsc);
}

static void draw_frame(lv_timer_t *t)
{

  if (draw_r1_enabled) draw_r1();
  if (draw_r2_enabled) draw_r2();
  if (draw_r3_enabled) draw_r3();
  if (draw_r4_enabled) draw_r4();
}

/////////

// Event callback for screen taps
static void screen_event_cb(lv_event_t * e) {
    Serial.println("-> screen_event_cb called"); // DEBUG: Check if event handler is triggered at all
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * screen = lv_event_get_target(e); // Get the object that triggered the event (the screen)

    Serial.printf("   Event code: %d (LV_EVENT_CLICKED is %d)\n", code, LV_EVENT_CLICKED); // DEBUG: Check event code

    if (code == LV_EVENT_CLICKED) {
        Serial.println("   LV_EVENT_CLICKED detected!"); // DEBUG

        // Check if an IP label already exists (optional, prevents multiple labels)
        lv_obj_t * old_label = lv_obj_get_child(screen, -1); // Get the topmost child
        // A simple check if it might be our label (could be more robust)
        if (old_label && lv_obj_check_type(old_label, &lv_label_class)) {
             Serial.println("   An old IP label might exist. Skipping creation."); // DEBUG
             // Optional: delete previous label immediately or just return
             // lv_obj_del(old_label);
              return; // Or just don't create a new one if one exists
        }


        Serial.println("   Creating new IP label..."); // DEBUG
        // Create a new label for the IP address
        lv_obj_t * ip_label = lv_label_create(screen);
        if (!ip_label) {
            Serial.println("   ERROR: Failed to create lv_label!"); // DEBUG
            return;
        }
        Serial.printf("   Setting label text to: %s\n", ip_address_str); // DEBUG
        lv_label_set_text(ip_label, ip_address_str); // Use the global IP string
        lv_obj_set_style_text_align(ip_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_bg_color(ip_label, lv_color_black(), 0); // Add background for visibility
        lv_obj_set_style_bg_opa(ip_label, LV_OPA_70, 0);
        lv_obj_set_style_text_color(ip_label, lv_color_white(), 0);
        lv_obj_set_style_pad_all(ip_label, 5, 0); // Add some padding
        lv_obj_align(ip_label, LV_ALIGN_CENTER, 0, 0); // Align to center

        Serial.println("   Creating one-shot timer for label deletion..."); // DEBUG
        // Create a one-shot timer to delete the label after 5 seconds (5000 ms)
        lv_timer_t* del_timer = lv_timer_create(ip_label_delete_timer_cb, 5000, ip_label);
        if (!del_timer) {
             Serial.println("   ERROR: Failed to create deletion timer!"); // DEBUG
        }
        lv_timer_set_repeat_count(del_timer, 1); // Ensure it's one-shot

    } else {
         Serial.println("   Event was not LV_EVENT_CLICKED."); // DEBUG
    }
}

void sketch_setup()
{
  // Allocate canvas in PSRAM
  cbuf = (lv_color_t *)heap_caps_malloc(
      LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_HEIGHT),
      MALLOC_CAP_SPIRAM);

  if (!cbuf)
  {
    Serial.println("❌ Failed to allocate canvas buffer in PSRAM");
    strncpy(ip_address_str, "Canvas Error", 15); // Update status
    ip_address_str[15] = '\0';
  }

  lv_obj_t *scr = lv_scr_act();

  // --- Keep touch setup for later debugging ---
  Serial.println("Adding screen click event callback..."); // DEBUG
  lv_obj_add_event_cb(scr, screen_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE); // Ensure the screen is clickable
  Serial.println("Screen click event callback added."); // DEBUG
  // --- End touch setup ---


  // Create the canvas if buffer allocation succeeded
  if (cbuf) {
      canvas = lv_canvas_create(scr);
      lv_canvas_set_buffer(canvas, cbuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
      lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);
      lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);
      Serial.println("Canvas created."); // DEBUG
      // Optional: Make canvas non-clickable if you want clicks to go "through" to the screen
      // lv_obj_clear_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
  } else {
      // Optional: Create a simple error label if canvas failed
      Serial.println("Canvas buffer allocation failed, creating error label."); // DEBUG
      lv_obj_t * error_label = lv_label_create(scr);
      lv_label_set_text(error_label, "Error:\nCanvas alloc failed!");
      lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
  }


  // Set a timer to draw generatively like a sketch loop
  Serial.println("Creating draw_frame timer..."); // DEBUG
  lv_timer_create(draw_frame, UPDATE_PERIOD, NULL);
  Serial.println("draw_frame timer created."); // DEBUG


  // --- Automatically display IP label on startup ---
  Serial.println("Creating IP label automatically..."); // DEBUG
  lv_obj_t * ip_label = lv_label_create(scr); // Create label on the screen
  if (!ip_label) {
      Serial.println("   ERROR: Failed to create lv_label for auto display!"); // DEBUG
  } else {
      Serial.printf("   Setting auto label text to: %s\n", ip_address_str); // DEBUG
      lv_label_set_text(ip_label, ip_address_str); // Use the global IP string
      lv_obj_set_style_text_align(ip_label, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_style_bg_color(ip_label, lv_color_black(), 0); // Add background for visibility
      lv_obj_set_style_bg_opa(ip_label, LV_OPA_70, 0);
      lv_obj_set_style_text_color(ip_label, lv_color_white(), 0);
      lv_obj_set_style_pad_all(ip_label, 5, 0); // Add some padding
      lv_obj_align(ip_label, LV_ALIGN_CENTER, 0, 0); // Align to center
      lv_obj_move_foreground(ip_label); // Ensure label is drawn on top of canvas

      Serial.println("   Creating one-shot timer for auto label deletion (10s)..."); // DEBUG
      // Create a one-shot timer to delete the label after 10 seconds (10000 ms)
      lv_timer_t* del_timer = lv_timer_create(ip_label_delete_timer_cb, 10000, ip_label);
      if (!del_timer) {
           Serial.println("   ERROR: Failed to create auto deletion timer!"); // DEBUG
      }
      lv_timer_set_repeat_count(del_timer, 1); // Ensure it's one-shot
  }
  // --- End automatic IP label display ---

}

void sketch_loop()
{
  // --- Message parsing for text commands ---
  // Allow repeated commands (e.g. clear, r1 on, r1 off, etc.)
  check_image_update(); // Check for new image updates

  // Parse text commands for toggling draw algorithms and clear
  if (strcmp(ws_text_value, "clear") == 0) {
      // Clear the canvas to white
      if (canvas && cbuf) {
          lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);
      }
      // Optionally clear the image widget as well
      if (img_widget) {
          lv_img_set_src(img_widget, NULL);
      }
      Serial.println("[Sketch] Received 'clear' command, screen cleared.");
  } else if (strncmp(ws_text_value, "r0 on", 5) == 0) {
      draw_r0_enabled = true;
      Serial.println("[Sketch] r0 enabled");
  } else if (strncmp(ws_text_value, "r0 off", 6) == 0) {
      draw_r0_enabled = false;
      Serial.println("[Sketch] r0 disabled");
  } else if (strncmp(ws_text_value, "r1 on", 5) == 0) {
      draw_r1_enabled = true;
      Serial.println("[Sketch] r1 enabled");
  } else if (strncmp(ws_text_value, "r1 off", 6) == 0) {
      draw_r1_enabled = false;
      Serial.println("[Sketch] r1 disabled");
  } else if (strncmp(ws_text_value, "r2 on", 5) == 0) {
      draw_r2_enabled = true;
      Serial.println("[Sketch] r2 enabled");
  } else if (strncmp(ws_text_value, "r2 off", 6) == 0) {
      draw_r2_enabled = false;
      Serial.println("[Sketch] r2 disabled");
  } else if (strncmp(ws_text_value, "r3 on", 5) == 0) {
      draw_r3_enabled = true;
      Serial.println("[Sketch] r3 enabled");
  } else if (strncmp(ws_text_value, "r3 off", 6) == 0) {
      draw_r3_enabled = false;
      Serial.println("[Sketch] r3 disabled");
  } else if (strncmp(ws_text_value, "r4 on", 5) == 0) {
      draw_r4_enabled = true;
      Serial.println("[Sketch] r4 enabled");
  } else if (strncmp(ws_text_value, "r4 off", 6) == 0) {
      draw_r4_enabled = false;
      Serial.println("[Sketch] r4 disabled");
  }
}


