#include "I2C_Driver.h"
#include <stdio.h> // Include for printf

void I2C_Init(void) {
  Wire.begin( I2C_SDA_PIN, I2C_SCL_PIN);
}
// 寄存器地址为 8 位的
bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);
  // Use endTransmission(false) to keep the connection active for the read
  uint8_t result = Wire.endTransmission(false);
  if (result != 0) {
    // Print detailed error
    printf("I2C Read Fail (Write Reg Addr): Addr=0x%02X, Reg=0x%02X, Error=%d\r\n", Driver_addr, Reg_addr, result);
    return false; // Return false on failure
  }

  // Request data
  uint8_t bytesReceived = Wire.requestFrom(Driver_addr, Length);
  if (bytesReceived != Length) {
      // Optional: Add check for incorrect number of bytes received
      printf("I2C Read Fail (Request Data): Addr=0x%02X, Req=%d, Recv=%d\r\n", Driver_addr, Length, bytesReceived);
      // Read the available bytes anyway to clear the buffer, but report failure
      for (int i = 0; i < bytesReceived; i++) {
          *Reg_data++ = Wire.read();
      }
      return false; // Return false on failure
  }

  // Read data
  for (int i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  return true; // Return true on success
}

bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  uint8_t result = Wire.endTransmission(true); // Send STOP condition
  if (result != 0)
  {
    // Print detailed error
    printf("I2C Write Fail: Addr=0x%02X, Reg=0x%02X, Error=%d\r\n", Driver_addr, Reg_addr, result);
    return false; // Return false on failure
  }
  return true; // Return true on success
}