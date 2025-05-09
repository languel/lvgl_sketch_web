# espixies

ESP32 LVGL WebSocket Generative Experiments

---


This project is an ESP32-based application that uses LVGL to create an interactive visual display. It connects to a Wi-Fi network and a WebSocket server to receive commands and data, which dynamically alter the visuals.

**Key functionalities include:**

* **WebSocket Communication (`lvgl_sketch_web.ino`):**
  * Receives `slider` (float for opacity), `number` (float for size/thickness), `text` (for commands like `rX on/off`, `clear`, and temporary on-screen display), and `image` (base64 encoded for background) messages.
* **Drawing Logic (`sketch.cpp`):**
  * **Image Background (`r0`):** Decodes and displays a scaled base64 image on the canvas.
  * **Generative Layers (`r1`-`r3`):** Draws lines, triangles, and arcs. `r4` is a placeholder. These layers can be toggled on/off via WebSocket commands.
  * Appearance (opacity/size) of generative art is controlled by the `slider` and `number` values.
* **LVGL & Hardware:** Uses LVGL for rendering, with specific drivers for an ST7701 display and TCA9554PWR I/O.
* **Memory:** Decoded images are stored in PSRAM.

The goal is to have a flexible system for remote-controlled visual art.

---

## Drawing Algorithm Control via WebSocket

You can control which generative art algorithms are active on the ESP32 device by sending text commands over WebSocket. These commands are parsed in real time and toggle the drawing routines:

### Drawing Commands

* `r0 on` / `r0 off` — Enable/disable the image background (JPEG sent via WebSocket as Base64).
* `r1 on` / `r1 off` — Enable/disable random lines (opacity and thickness controlled by slider/number).
* `r2 on` / `r2 off` — Enable/disable random triangles (opacity and size controlled by slider/number).
* `r3 on` / `r3 off` — Enable/disable random arcs (opacity and thickness controlled by slider/number).
* `r4 on` / `r4 off` — Enable/disable extra drawing mode (customizable).
* `r5 on` / `r5 off` — Enable/disable pointillist image rendering. Decodes the last received image and redraws it as a grid of circles.
    *   The color of each circle corresponds to a pixel in the source image.
    *   `slider` controls the opacity of the circles.
    *   `number` controls the radius of the circles relative to their grid cell size (e.g., a larger number makes bigger circles).
* `clear` — Immediately clears the canvas and removes the image background.

You can send these commands repeatedly; each will be processed every time.

### Example Usage

* To show only the image background: `r0 on`, `r1 off`, `r2 off`, `r3 off`, `r4 off`
* To enable lines and triangles: `r1 on`, `r2 on`
* To clear the screen: `clear`

### WebSocket Message Format

Send a JSON message of type `text` with the command as the value:

```json
{ "type": "text", "value": "r2 on" }
```

Or use the provided web UI to send commands directly.

## LVGL WebSocket Generative Art Demo

This project demonstrates a generative art application running on an ESP32 with a display, using the LVGL graphics library. The device connects to Wi-Fi and acts as a WebSocket client, allowing real-time control of the visuals from a web interface or other WebSocket server. It also supports receiving and displaying small images sent as Base64-encoded JPEGs.

## Quick Start Configuration

To get the project running with your setup, you need to modify **`lvgl_sketch_web.ino`**:

1. **Wi-Fi Network:**

   Update the following lines with your Wi-Fi credentials:

   ```cpp
   const char *WIFI_SSID = "YOUR_WIFI_SSID"; // Replace with your Wi-Fi SSID
   const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"; // Replace with your Wi-Fi Password
   ```

2. **WebSocket Server IP Address:**

   Update the WebSocket server details:

   ```cpp
   const char* WEBSOCKET_SERVER_IP = "YOUR_WEBSOCKET_SERVER_IP"; // Replace with your server's IP address
   const uint16_t WEBSOCKET_SERVER_PORT = 5001; // default=5001. Change if needed to this matches your server's port 
   ```

Once these are set, you can proceed with building and uploading the project.

## Features

* **LVGL Canvas Drawing:** Generative art is rendered on an LVGL canvas, with parameters controlled remotely.
* **Wi-Fi Connection:** The ESP32 connects to a specified Wi-Fi network on boot.
* **WebSocket Client:** The device connects to a WebSocket server (e.g., TouchDesigner, Node.js, or Python server) and listens for control messages.
* **Real-Time Control:** Supports remote control of drawing parameters via messages of type `slider`, `number`, and `text`.
* **Temporary Text Display:** Incoming text messages are displayed on the screen for a few seconds, with font size controlled by the slider.
* **Image Display:** Supports receiving small JPEG images as Base64 strings and displaying them on the screen (requires LVGL JPEG decoder enabled).

## File Structure

* `lvgl_sketch_web.ino` — Main Arduino sketch. Handles Wi-Fi, WebSocket, and event dispatch.
* `sketch.cpp` / `sketch.h` — Generative art logic, LVGL canvas setup, and UI event handling.
* `base64_utils.cpp` / `base64_utils.h` — Lightweight Base64 decoder for handling image data.
* `Display_ST7701.*`, `LVGL_Driver.*`, `TCA9554PWR.*`, etc. — Hardware and display drivers.
* `webui/` — Contains the web interface (e.g., `index.html`) for controlling the device.
* `.gitignore` — Standard ignores for Arduino/C++/PlatformIO projects.
* `touchdesigner/` — Contains an example TouchDesigner project (`td-sockets.toe`) that can act as a WebSocket server.

## Usage

### 1. Hardware Requirements

* ESP32 board with PSRAM (recommended for LVGL canvas and image buffers)
* ST7701-based display (or compatible, as per your drivers)
* Touch input optional (for future expansion)

### 2. Building and Uploading

* Open the project in Arduino IDE or PlatformIO.
* Select the correct ESP32 board and port.
* Build and upload as usual.

### 3. WebSocket Server

* You can use the included `webui/index.html` as a web client, or run a compatible WebSocket server (e.g., TouchDesigner, Node.js, Python). An example TouchDesigner project (`td-sockets.toe`) is provided in the `touchdesigner/` folder.
* The server should:
  * Accept connections on the specified port.
  * Send JSON messages of the form:
    * `{ "type": "slider", "value": 0.5 }`
    * `{ "type": "number", "value": 2 }`
    * `{ "type": "text", "value": "Hello!" }`
    * `{ "type": "image", "mime": "image/jpeg", "data": "...base64..." }`

### 4. Controls

* **Slider:** Controls a visual parameter (e.g., line width, font size).
* **Number:** Controls another parameter (e.g., arc width, position range).
* **Text:** Displays a temporary label in the center of the screen, font size set by slider.
* **Image:** If a small JPEG is sent as Base64, it will be decoded and displayed (see notes below).

### 5. Image Support Notes

* Only small JPEGs (e.g., 16x16) are supported due to memory and performance constraints.
* LVGL must be configured with JPEG decoder support (`LV_USE_LIBJPEG_TURBO` or `LV_USE_TJPGD` in `lv_conf.h`).
* The Base64 decoder is custom and expects standard Base64 encoding.

### 6. Extending

* You can add more message types or controls by expanding the JSON parsing in `webSocketEvent` and updating the drawing logic in `sketch.cpp`.
* Touch input can be enabled for local interaction.

## Troubleshooting

* If the device reboots or crashes, check for memory issues or conflicts between Wi-Fi and LVGL initialization order.
* If images do not display, ensure JPEG decoder is enabled in LVGL and the image is small enough to fit the buffer.
* If the WebSocket connection drops when sending large images, reduce image size or send less frequently.

## License

This project is for educational and prototyping use. See individual library licenses for LVGL and other dependencies.
