// #include <SPIFFS.h>
// #include <JPEGDEC.h>

// JPEGDEC jpeg;

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   if (!SPIFFS.begin(true)) {
//     Serial.println("SPIFFS Mount Failed!");
//     return;
//   }

//   File jpgFile = SPIFFS.open("/assets/testimg16.jpg", "r");
//   if (!jpgFile) {
//     Serial.println("Failed to open JPEG file!");
//     return;
//   }

//   size_t jpgSize = jpgFile.size();
//   uint8_t* jpgBuf = (uint8_t*)malloc(jpgSize);
//   if (!jpgBuf) {
//     Serial.println("Failed to allocate buffer!");
//     jpgFile.close();
//     return;
//   }
//   jpgFile.read(jpgBuf, jpgSize);
//   jpgFile.close();

//   int rc = jpeg.openRAM(jpgBuf, jpgSize, NULL);
//   if (rc != JPEG_SUCCESS) {
//     Serial.println("JPEG openRAM failed!");
//     free(jpgBuf);
//     return;
//   }

//   Serial.printf("JPEG dimensions: %d x %d\n", jpeg.getWidth(), jpeg.getHeight());

//   // You can now decode to a buffer or draw using a callback (next step)
//   jpeg.close();
//   free(jpgBuf);
// }

// void loop() {}