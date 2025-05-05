#define LVGL_TICK_PERIOD 5

#include <WiFi.h>
#include <WebSocketsClient.h> // Include the WebSocket library
#include "TCA9554PWR.h"
#include "Display_ST7701.h"
#include "LVGL_Driver.h"
#include "sketch.h"
#include <ArduinoJson.h> // Include ArduinoJson library

const char *WIFI_SSID = "Pratt Institute";
const char *WIFI_PASSWORD = "";

// --- WebSocket Settings ---
const char* WEBSOCKET_SERVER_IP = "10.16.48.206"; // <-- IMPORTANT: Replace with your server's IP address
const uint16_t WEBSOCKET_SERVER_PORT = 5001;
WebSocketsClient webSocket;
bool isWebSocketConnected = false; // Track connection status
// --- End WebSocket Settings ---


// --- Global Control Variables ---
float ws_slider_value = 0.5f; // Default value
float ws_number_value = 1.0f; // Default value
char ws_text_value[64] = "default"; // Default value, ensure size matches sketch.h
// --- End Global Control Variables ---

// --- Temporary Text Label ---
static lv_obj_t* current_text_label = nullptr; // Pointer to the currently displayed text label
static lv_timer_t* text_delete_timer = nullptr; // Pointer to the deletion timer for the text label

// Timer callback to delete the temporary text label
static void text_label_delete_timer_cb(lv_timer_t * timer) {
    Serial.println("-> text_label_delete_timer_cb called"); // DEBUG
    lv_obj_t * label = (lv_obj_t *)timer->user_data;
    if(label) {
        Serial.println("   Deleting temporary text label..."); // DEBUG
        lv_obj_del(label);
    } else {
        Serial.println("   Text label object was NULL in timer callback."); // DEBUG
    }
    // Ensure we clear the pointers now that the label and timer are done
    current_text_label = nullptr;
    text_delete_timer = nullptr;
}
// --- End Temporary Text Label ---

char ip_address_str[16] = "Connecting...";

// --- WebSocket Event Handler ---
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            isWebSocketConnected = false;
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            isWebSocketConnected = true;
            // Optional: send message to server on connect
            webSocket.sendTXT("{\"client\":\"ESP32\"}"); // Example: Send identification
            break;
        case WStype_TEXT:
            { // Add braces for local variable scope
                Serial.printf("[WSc] Received text: %s\n", payload);

                // Parse JSON payload
                StaticJsonDocument<256> doc;
                DeserializationError error = deserializeJson(doc, payload, length);

                if (error) {
                    Serial.print(F("[JSON] deserializeJson() failed: "));
                    Serial.println(error.f_str());
                    return; // Exit if parsing fails
                }

                const char* msg_type = doc["type"];

                if (msg_type) {
                    if (strcmp(msg_type, "slider") == 0) {
                        // Check if "value" exists and is a number
                        if (doc["value"].is<float>() || doc["value"].is<int>()) {
                            ws_slider_value = doc["value"].as<float>();
                            Serial.printf("  [JSON] Parsed slider value: %f\n", ws_slider_value);
                        } else {
                             Serial.println("  [JSON] Slider message missing valid 'value'.");
                        }
                    } else if (strcmp(msg_type, "number") == 0) {
                         if (doc["value"].is<float>() || doc["value"].is<int>()) {
                            ws_number_value = doc["value"].as<float>();
                            Serial.printf("  [JSON] Parsed number value: %f\n", ws_number_value);
                        } else {
                             Serial.println("  [JSON] Number message missing valid 'value'.");
                        }
                    } else if (strcmp(msg_type, "text") == 0) {
                         if (doc["value"].is<const char*>()) {
                            strncpy(ws_text_value, doc["value"], sizeof(ws_text_value) - 1);
                            ws_text_value[sizeof(ws_text_value) - 1] = '\0'; // Ensure null termination
                            Serial.printf("  [JSON] Parsed text value: %s\n", ws_text_value);

                            // --- Display the received text temporarily ---
                            // 1. Delete previous label and its timer if they exist
                            if(text_delete_timer) {
                                lv_timer_del(text_delete_timer); // Delete the timer first
                                text_delete_timer = nullptr;
                            }
                            if(current_text_label) {
                                lv_obj_del(current_text_label); // Delete the label object
                                current_text_label = nullptr;
                            }

                            // 2. Determine font based on slider value
                            const lv_font_t *selected_font = &lv_font_montserrat_14; // Default (Available)
                            if (ws_slider_value >= 0.75f) {
                                // Use largest available suggested font
                                selected_font = &lv_font_montserrat_16; // Use 16 instead of 32
                            } else if (ws_slider_value >= 0.5f) {
                                // Use medium available suggested font
                                selected_font = &lv_font_montserrat_14; // Use 14 instead of 24 (already default, but explicit)
                            } else if (ws_slider_value >= 0.25f) {
                                // Use smallest available suggested font (other than default)
                                selected_font = &lv_font_montserrat_12; // Use 12 instead of 18
                            }
                            // Ensure selected_font points to a valid font enabled in lv_conf.h

                            // 3. Create the new label
                            lv_obj_t * new_label = lv_label_create(lv_scr_act());
                            if (new_label) {
                                current_text_label = new_label; // Store pointer to the new label
                                lv_label_set_text(new_label, ws_text_value);
                                lv_obj_set_style_text_font(new_label, selected_font, 0); // Set font size
                                lv_obj_set_style_text_align(new_label, LV_TEXT_ALIGN_CENTER, 0);
                                lv_obj_set_style_bg_color(new_label, lv_color_black(), 0);
                                lv_obj_set_style_bg_opa(new_label, LV_OPA_70, 0);
                                lv_obj_set_style_text_color(new_label, lv_color_white(), 0);
                                lv_obj_set_style_pad_all(new_label, 8, 0); // More padding for larger fonts
                                lv_obj_align(new_label, LV_ALIGN_CENTER, 0, 0);
                                lv_obj_move_foreground(new_label); // Draw on top

                                // 4. Create deletion timer
                                text_delete_timer = lv_timer_create(text_label_delete_timer_cb, 5000, new_label); // 5 seconds
                                if(text_delete_timer) {
                                    lv_timer_set_repeat_count(text_delete_timer, 1); // One-shot
                                } else {
                                     Serial.println("   ERROR: Failed to create text deletion timer!");
                                     // Clean up label if timer fails?
                                     lv_obj_del(new_label);
                                     current_text_label = nullptr;
                                }
                            } else {
                                Serial.println("   ERROR: Failed to create temporary text label!");
                            }
                            // --- End temporary text display ---

                        } else {
                             Serial.println("  [JSON] Text message missing valid 'value'.");
                        }
                    } else if (strcmp(msg_type, "connection") == 0) {
                        // Ignore the connection status message we receive now
                        Serial.println("  [JSON] Ignoring 'connection' message.");
                    }
                    else {
                        Serial.printf("  [JSON] Unknown message type: %s\n", msg_type);
                    }
                } else {
                     Serial.println("  [JSON] Received JSON without 'type' field.");
                }
            } // End of local scope for JSON parsing
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
  if (WiFi.status() == WL_CONNECTED) {
      if (strcmp(WEBSOCKET_SERVER_IP, "YOUR_WEBSOCKET_SERVER_IP") == 0) {
           Serial.println("\n[WSc] ERROR: WebSocket server IP not configured!");
           strncpy(ip_address_str, "WS IP Error", 15); // Update status
           ip_address_str[15] = '\0';
      } else {
          Serial.printf("[WSc] Connecting to WebSocket server: %s:%d\n", WEBSOCKET_SERVER_IP, WEBSOCKET_SERVER_PORT);
          // Connect to WebSocket server (path "/" is common)
          webSocket.begin(WEBSOCKET_SERVER_IP, WEBSOCKET_SERVER_PORT, "/");
          // Assign the event handler function
          webSocket.onEvent(webSocketEvent);
          // Set reconnect interval in ms (optional)
          webSocket.setReconnectInterval(5000); // try every 5 seconds
          // Start heartbeat (optional, helps keep connection alive)
          // webSocket.enableHeartbeat(15000, 3000, 2); // Send ping every 15s, timeout 3s, max 2 retries
      }
  } else {
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

  printf("\n *** Setup Complete *** \n\n");
}

void loop()
{
  webSocket.loop(); // MUST call this frequently to process WebSocket events
  Lvgl_Loop();      // LVGL loop that handles ticks and rendering
  sketch_loop();
  delay(LVGL_TICK_PERIOD); // Keep this delay small
}
