#define LVGL_TICK_PERIOD 5

#include <WiFi.h>
#include <WebSocketsClient.h> // Include the WebSocket library
#include "TCA9554PWR.h"
#include "Display_ST7701.h"
#include "LVGL_Driver.h"
#include "sketch.h"
#include <ArduinoJson.h> // Include ArduinoJson library
#include "base64_utils.h"
#include <JPEGDEC.h> // Include JPEG decoder library

// --- WebSocket and JPEG Decoding Globals ---
static JPEGDEC jpeg; // JPEG decoder instance
static uint16_t* g_jpeg_target_buffer = nullptr; // Target buffer for JPEG callback
static int g_jpeg_target_width = 0; // Target width for JPEG callback
static bool decoded_buffer_is_dynamic = false; // Tracks if decoded_img_buffer is on heap
// --- End WebSocket and JPEG Globals ---

// --- Connection Settings ---
const char *WIFI_SSID = "YOUR WIFI SSID"; // <-- IMPORTANT: Replace with your Wi-Fi SSID
const char *WIFI_PASSWORD = "YOUR WIFI PASSWORD";              // <-- IMPORTANT: Replace with your Wi-Fi Password
const char* WEBSOCKET_SERVER_IP = "YOUR WEBSOCKET SERVER"; // <-- IMPORTANT: Replace with your server's IP address e.g. Touchdesigner

const uint16_t WEBSOCKET_SERVER_PORT = 5001;       // <-- IMPORTANT: Replace with your server's port number if different
WebSocketsClient webSocket;
bool isWebSocketConnected = false; // Track connection status
// --- End WebSocket Settings ---


// --- Global Control Variables ---
float ws_slider_value = 0.5f;         // Default value
float ws_number_value = 1.0f;         // Default value
char ws_text_value[1024] = "default"; // Default value, ensure size matches sketch.h
int received_image_width = 0;         // To store width from WebSocket message
int received_image_height = 0;        // To store height from WebSocket message

// --- Image Buffer Globals ---
uint16_t test_pixel_buffer[16 * 16];
uint16_t *decoded_img_buffer = test_pixel_buffer; 
size_t decoded_img_size = sizeof(test_pixel_buffer);
// int decoded_img_width = 16; // MOVED TO SKETCH.CPP
// int decoded_img_height = 16; // MOVED TO SKETCH.CPP
volatile bool new_image_available = true; 
// --- End Global Control Variables ---

// JPEG Draw Callback function
// This function is called by the JPEGDEC library to draw pixels.
// We use it to copy pixel data into our global decoded_img_buffer.
int jpegDrawCallback(JPEGDRAW *pDraw) {
    if (!g_jpeg_target_buffer || g_jpeg_target_width == 0) {
        Serial.println("[jpegDrawCallback] Error: Target buffer or width not set!");
        return 0; // Stop decoding if setup is incorrect
    }

    uint16_t *src_pixels = pDraw->pPixels; // Pixels from the current MCU

    for (int y = 0; y < pDraw->iHeight; y++) {
        for (int x = 0; x < pDraw->iWidth; x++) {
            int dest_x = pDraw->x + x;
            int dest_y = pDraw->y + y;

            // Ensure we are within the bounds of our target buffer
            // decoded_img_height is a global variable updated before decode starts
            if (dest_x < g_jpeg_target_width && dest_y < decoded_img_height) {
                g_jpeg_target_buffer[dest_y * g_jpeg_target_width + dest_x] = src_pixels[y * pDraw->iWidth + x];
            } else {
                // This might happen if JPEG dimensions are slightly off or MCU overlaps boundary
                // Serial.printf("[jpegDrawCallback] Warning: Pixel out of bounds (%d, %d)\n", dest_x, dest_y);
            }
        }
    }
    return 1; // Return 1 to continue decoding
}

// Helper to initialize the test pixel buffer with random colors
void init_test_pixel_buffer() {
    randomSeed(analogRead(0));
    for (int i = 0; i < 16 * 16; ++i) {
        test_pixel_buffer[i] = random(0, 0xFFFF);
    }
    decoded_img_buffer = test_pixel_buffer; 
    // decoded_img_width = 16; // Will use the one from sketch.cpp, initialized there
    // decoded_img_height = 16; // Will use the one from sketch.cpp, initialized there
    // Ensure sketch.cpp initializes these to 16 for the test buffer initially
    decoded_img_size = sizeof(test_pixel_buffer); 
    new_image_available = true;
    Serial.println("[InitTestPixelBuffer] Test pixel buffer initialized and new_image_available set to true.");
}

// --- Temporary Text Label ---
static lv_obj_t *current_text_label = nullptr;  // Pointer to the currently displayed text label
static lv_timer_t *text_delete_timer = nullptr; // Pointer to the deletion timer for the text label

// Timer callback to delete the temporary text label
static void text_label_delete_timer_cb(lv_timer_t *timer)
{
    Serial.println("-> text_label_delete_timer_cb called"); // DEBUG
    lv_obj_t *label = (lv_obj_t *)timer->user_data;
    if (label)
    {
        Serial.println("   Deleting temporary text label..."); // DEBUG
        lv_obj_del(label);
    }
    else
    {
        Serial.println("   Text label object was NULL in timer callback."); // DEBUG
    }
    // Ensure we clear the pointers now that the label and timer are done
    current_text_label = nullptr;
    text_delete_timer = nullptr;
}
// --- End Temporary Text Label ---

char ip_address_str[16] = "Connecting...";

// --- WebSocket Event Handler ---
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[WSc] Disconnected!\n");
        isWebSocketConnected = false;
        break;
    case WStype_CONNECTED:
        Serial.printf("[WSc] Connected to url: %s\n", payload);
        isWebSocketConnected = true;
        break;
    case WStype_TEXT:
    { 
        DynamicJsonDocument doc(6 * 1024 * 1024); // Increased capacity for potentially large base64
        DeserializationError error = deserializeJson(doc, payload, length);

        if (error)
        {
            Serial.print(F("[WSc] deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }

        const char *msg_type = doc["type"];

        if (msg_type)
        {
            if (strcmp(msg_type, "slider") == 0)
            {
                ws_slider_value = doc["value"].as<float>();
                // Serial.printf("[WSc] Slider value: %f\n", ws_slider_value);
            }
            else if (strcmp(msg_type, "number") == 0)
            {
                ws_number_value = doc["value"].as<float>();
                // Serial.printf("[WSc] Number value: %f\n", ws_number_value);
            }
            else if (strcmp(msg_type, "text") == 0)
            {
                const char *txt = doc["value"];
                if (txt) {
                    strncpy(ws_text_value, txt, sizeof(ws_text_value) - 1);
                    ws_text_value[sizeof(ws_text_value) - 1] = '\0'; // Ensure null termination
                    // Serial.printf("[WSc] Text value: %s\n", ws_text_value);
                    // display_temporary_text(ws_text_value); // Assuming this function exists and is defined elsewhere
                }
            }
            else if (strcmp(msg_type, "image") == 0)
            {
                // Serial.println("[WSc] 'image' message type identified."); // Confirm this block is reached
                // Print the raw payload for debugging
                // Serial.printf("[WSc] Raw payload for 'image' (length %zu): %.*s\n", length, (int)length, (char*)payload);

                if (doc.containsKey("data")) { // MODIFIED: Check for "data" key
                    JsonVariant image_val = doc["data"]; // MODIFIED: Get "data" key
                    if (image_val.isNull()) {
                        // Serial.println("[WSc] JSON 'data' field is null."); // MODIFIED: Log for "data"
                    } else if (image_val.is<const char*>()) {
                        const char* b64_data = image_val.as<const char*>();
                        // Serial.printf("[WSc] JSON 'data' field (string) starts with: %.*s...\n", 30, b64_data ? b64_data : "NULL_PTR"); // MODIFIED: Log for "data"
                        if (b64_data) {
                            // Serial.printf("[WSc] JSON 'data' field string length: %d\n", strlen(b64_data)); // MODIFIED: Log for "data"
                        }
                    } else {
                        // Serial.printf("[WSc] JSON 'data' field is not a string. Actual type: %s\n", image_val.is<int>() ? "int" : image_val.is<float>() ? "float" : image_val.is<bool>() ? "bool" : image_val.is<JsonArray>() ? "array" : image_val.is<JsonObject>() ? "object" : "unknown"); // MODIFIED: Log for "data"
                    }
                } else {
                    // Serial.println("[WSc] JSON 'data' field is missing for 'image' type."); // MODIFIED: Log for "data"
                }

                // Extract width and height if present
                if (doc.containsKey("width") && doc["width"].is<int>()) {
                    received_image_width = doc["width"].as<int>();
                } else {
                    received_image_width = 0; // Default or indicate error
                }
                if (doc.containsKey("height") && doc["height"].is<int>()) {
                    received_image_height = doc["height"].as<int>();
                } else {
                    received_image_height = 0; // Default or indicate error
                }
                // Optional: Log received dimensions
                // Serial.printf("[WSc] Received image dimensions: %d x %d\n", received_image_width, received_image_height);


                const char *base64_image_data = doc["data"]; 
                if (base64_image_data) {
                    // Serial.println("[WSc] Received image data (base64_image_data is not null). Decoding...");
                    size_t b64_decoded_len;
                    uint8_t *jpeg_raw_data = base64_decode_to_psram(base64_image_data, &b64_decoded_len);

                    if (jpeg_raw_data && b64_decoded_len > 0) {
                        // Serial.printf("[JPEG] Base64 decoded to %d bytes in PSRAM.\n", b64_decoded_len);

                        if (jpeg.openRAM(jpeg_raw_data, b64_decoded_len, jpegDrawCallback)) {
                            // Serial.println("[JPEG] RAM buffer opened.");
                            jpeg.setPixelType(RGB565_LITTLE_ENDIAN); // Critical for LVGL compatibility

                            int new_img_width = jpeg.getWidth();
                            int new_img_height = jpeg.getHeight();
                            // Serial.printf("[JPEG] Dimensions: %d x %d\n", new_img_width, new_img_height);

                            if (new_img_width > 0 && new_img_height > 0) {
                                size_t new_buffer_byte_size = (size_t)new_img_width * new_img_height * sizeof(uint16_t);
                                uint16_t* temp_new_pixel_buffer = (uint16_t*)heap_caps_malloc(new_buffer_byte_size, MALLOC_CAP_SPIRAM);

                                if (temp_new_pixel_buffer) {
                                    // Serial.printf("[JPEG] Allocated %zu bytes for new pixel buffer in PSRAM.\n", new_buffer_byte_size);

                                    if (decoded_buffer_is_dynamic && decoded_img_buffer != nullptr && decoded_img_buffer != test_pixel_buffer) {
                                        // Serial.println("[JPEG] Freeing previous dynamic image buffer.");
                                        heap_caps_free(decoded_img_buffer);
                                    }

                                    decoded_img_buffer = temp_new_pixel_buffer;
                                    decoded_img_width = new_img_width;
                                    decoded_img_height = new_img_height; // Set before decode for callback
                                    decoded_img_size = new_buffer_byte_size;
                                    decoded_buffer_is_dynamic = true;

                                    g_jpeg_target_buffer = decoded_img_buffer;
                                    g_jpeg_target_width = decoded_img_width;

                                    // Serial.println("[JPEG] Starting decode process...");
                                    if (jpeg.decode(0, 0, 0)) {
                                        // Serial.println("[JPEG] Decode successful.");
                                        new_image_available = true;
                                    } else {
                                        // Serial.println("[JPEG] Decode FAILED!");
                                        heap_caps_free(decoded_img_buffer); // Free the just-allocated buffer
                                        // Revert to test buffer
                                        decoded_img_buffer = test_pixel_buffer;
                                        decoded_img_width = 16;
                                        decoded_img_height = 16;
                                        decoded_img_size = sizeof(test_pixel_buffer);
                                        decoded_buffer_is_dynamic = false;
                                        new_image_available = true; // Show test image
                                    }
                                } else {
                                    // Serial.println("[JPEG] Failed to allocate PSRAM for new pixel buffer!");
                                }
                            } else {
                                // Serial.println("[JPEG] Header invalid or image dimensions are zero.");
                            }
                            jpeg.close();
                            // Serial.println("[JPEG] Closed.");
                        } else {
                            // Serial.println("[JPEG] jpeg.openRAM() failed!");
                        }
                        heap_caps_free(jpeg_raw_data); // Free the base64 decoded data
                        // Serial.println("[JPEG] Freed base64 decoded data buffer.");
                    } else {
                        // Serial.println("[JPEG] Base64 decoding failed or produced zero length data.");
                        if (jpeg_raw_data) heap_caps_free(jpeg_raw_data); // Safety free
                    }
                    // Clear global helpers
                    g_jpeg_target_buffer = nullptr;
                    g_jpeg_target_width = 0;
                } else {
                    // Serial.println("[WSc] Debug: base64_image_data (from doc's 'data' field) is null."); // MODIFIED: Log for "data"
                }
            }
            // ... any other message types ...
        }
        else
        {
            Serial.println("[WSc] Received JSON without 'type' field.");
        }
    } 
    break;
    case WStype_BIN:
        Serial.printf("[WSc] Received binary data of length: %u\n", length);
        break;
    case WStype_ERROR:
        // Explicitly log errors
        Serial.printf("[WSc] Error: %s\n", payload);
        break;
    case WStype_FRAGMENT_TEXT_START:
        Serial.println("[WSc] Fragmented text start");
        break;
    case WStype_FRAGMENT_BIN_START:
        Serial.println("[WSc] Fragmented bin start");
        break;
    case WStype_FRAGMENT:
        Serial.println("[WSc] Fragment received");
        break;
    case WStype_FRAGMENT_FIN:
        Serial.println("[WSc] Fragment fin");
        break;
    case WStype_PING:
        Serial.println("[WSc] Received ping");
        break;
    case WStype_PONG:
        Serial.println("[WSc] Received pong");
        break;
    default:
        Serial.printf("[WSc] Unknown Event: %d\n", type);
        break;
    }
}
// --- End WebSocket Event Handler ---

void testWiFi()
{
    Serial.printf("Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint8_t attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20)
    {
        delay(500);
        Serial.print('.');
        attempt++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("\nWiFi ✔  IP: %s\n\n", WiFi.localIP().toString().c_str());
        strncpy(ip_address_str, WiFi.localIP().toString().c_str(), 15);
        ip_address_str[15] = '\0';
    }
    else
    {
        Serial.println("\nWiFi ✘  failed to connect");
        strncpy(ip_address_str, "No Connection", 15);
        ip_address_str[15] = '\0';
    }
}

void setup()
{
    Serial.begin(115200);
    delay(100);
    printf("\n\n *** Starting Setup *** \n\n");

    // 1. Test Wi-Fi connection first
    testWiFi();

    // --- Start WebSocket Client (only if WiFi connected) ---
    if (WiFi.status() == WL_CONNECTED)
    {
        if (strcmp(WEBSOCKET_SERVER_IP, "YOUR_WEBSOCKET_SERVER_IP") == 0)
        {
            Serial.println("\n[WSc] ERROR: WebSocket server IP not configured!");
            strncpy(ip_address_str, "WS IP Error", 15); // Update status
            ip_address_str[15] = '\0';
        }
        else
        {
            Serial.printf("[WSc] Connecting to WebSocket server: %s:%d\n", WEBSOCKET_SERVER_IP, WEBSOCKET_SERVER_PORT);
            // Connect to WebSocket server (path "/" is common)
            webSocket.begin(WEBSOCKET_SERVER_IP, WEBSOCKET_SERVER_PORT, "/");
            // Assign the event handler function
            webSocket.onEvent(webSocketEvent);
            // Set reconnect interval in ms (optional)
            webSocket.setReconnectInterval(10000); // try every 10 seconds
                                                   //   Start heartbeat (optional, helps keep connection alive)
                                                   //   webSocket.enableHeartbeat(15000, 3000, 2); // Send ping every 15s, timeout 3s, max 2 retries
        }
    }
    else
    {
        Serial.println("[WSc] WiFi not connected, skipping WebSocket connection.");
    }
    // --- End Start WebSocket Client ---

    // 2. Initialize hardware peripherals needed for display/touch
    I2C_Init();
    TCA9554PWR_Init(0x00);
    Set_EXIO(EXIO_PIN8, Low);

    // 3. Initialize Display and LVGL *after* Wi-Fi attempt
    LCD_Init();
    Lvgl_Init();

    // 4. Setup the sketch UI
    sketch_setup();

    // 5. Initialize the test pixel buffer with random colors
    init_test_pixel_buffer();

    printf("\n *** Setup Complete *** \n\n");
}

void loop()
{
    webSocket.loop(); // MUST call this frequently to process WebSocket events
    Lvgl_Loop();      // LVGL loop that handles ticks and rendering
    sketch_loop();
    delay(LVGL_TICK_PERIOD); // Keep this delay small
}
