#line 1 "/Users/liubo/Documents/Arduino/examples/lvgl_sketch_web/TCA9554PWR.cpp"
#include "TCA9554PWR.h"
#include <stdio.h> // Include for printf

/*****************************************************  Operation register REG   ****************************************************/
// Read the value of the TCA9554PWR register REG
uint8_t I2C_Read_EXIO(uint8_t REG)
{
  Wire.beginTransmission(TCA9554_ADDRESS);
  Wire.write(REG);
  // Use endTransmission(false) for standard register read sequence
  uint8_t result = Wire.endTransmission(false);
  if (result != 0) {
    // Print detailed error
    printf("TCA9554 Read Fail (Write Reg): Addr=0x%02X, Reg=0x%02X, Error=%d\r\n", TCA9554_ADDRESS, REG, result);
    return 0xFF; // Return an error value (e.g., 0xFF)
  }

  uint8_t bytesReceived = Wire.requestFrom(TCA9554_ADDRESS, (uint8_t)1);
  if (bytesReceived != 1) {
      printf("TCA9554 Read Fail (Request Data): Addr=0x%02X, Req=1, Recv=%d\r\n", TCA9554_ADDRESS, bytesReceived);
      // Attempt to read anyway to clear buffer if needed
      if (bytesReceived > 0) Wire.read();
      return 0xFF; // Return an error value
  }

  uint8_t bitsStatus = Wire.read();
  return bitsStatus;
}

// Write Data to the REG register of the TCA9554PWR
// Returns 0 on success, non-zero on failure
uint8_t I2C_Write_EXIO(uint8_t REG,uint8_t Data)
{
  Wire.beginTransmission(TCA9554_ADDRESS);
  Wire.write(REG);
  Wire.write(Data);
  uint8_t result = Wire.endTransmission(true); // Send STOP
  if (result != 0) {
    // Print detailed error
    printf("TCA9554 Write Fail: Addr=0x%02X, Reg=0x%02X, Error=%d\r\n", TCA9554_ADDRESS, REG, result);
    return result; // Return the error code
  }
  return 0; // Return 0 for success
}

/********************************************************** Set EXIO mode **********************************************************/
// Set the mode of the TCA9554PWR Pin. State: 0= Output mode 1= input mode
void Mode_EXIO(uint8_t Pin,uint8_t State)
{
  uint8_t bitsStatus = I2C_Read_EXIO(TCA9554_CONFIG_REG);
  if (bitsStatus == 0xFF) { // Check if read failed
      printf("TCA9554 Mode_EXIO Fail: Could not read config reg\r\n");
      return;
  }

  uint8_t Data;
  if (State == 1) { // Input mode
      Data = (0x01 << (Pin-1)) | bitsStatus;
  } else { // Output mode
      Data = bitsStatus & ~(0x01 << (Pin-1));
  }

  uint8_t result = I2C_Write_EXIO(TCA9554_CONFIG_REG, Data);
  if (result != 0) {
    // Error already printed by I2C_Write_EXIO
    printf("TCA9554 Mode_EXIO Fail: Could not write config reg\r\n");
  }
}

// Set the mode of the 7 pins from the TCA9554PWR with PinState
void Mode_EXIOS(uint8_t PinState)
{
  uint8_t result = I2C_Write_EXIO(TCA9554_CONFIG_REG, PinState);
  if (result != 0) {
    // Error already printed by I2C_Write_EXIO
    printf("TCA9554 Mode_EXIOS Fail: Could not write config reg\r\n");
  }
}

/********************************************************** Read EXIO status **********************************************************/       
uint8_t Read_EXIO(uint8_t Pin)                            // Read the level of the TCA9554PWR Pin
{
  uint8_t inputBits = I2C_Read_EXIO(TCA9554_INPUT_REG);          
  uint8_t bitStatus = (inputBits >> (Pin-1)) & 0x01; 
  return bitStatus;                                  
}
uint8_t Read_EXIOS(uint8_t REG = TCA9554_INPUT_REG)       // Read the level of all pins of TCA9554PWR, the default read input level state, want to get the current IO output state, pass the parameter TCA9554_OUTPUT_REG, such as Read_EXIOS(TCA9554_OUTPUT_REG);
{
  uint8_t inputBits = I2C_Read_EXIO(REG);                     
  return inputBits;     
}

/********************************************************** Set the EXIO output status **********************************************************/  
void Set_EXIO(uint8_t Pin,uint8_t State)                  // Sets the level state of the Pin without affecting the other pins
{
  uint8_t Data;
  if(State < 2 && Pin < 9 && Pin > 0){  
    uint8_t bitsStatus = Read_EXIOS(TCA9554_OUTPUT_REG);
    if(State == 1)                                     
      Data = (0x01 << (Pin-1)) | bitsStatus; 
    else if(State == 0)                  
      Data = (~(0x01 << (Pin-1))) & bitsStatus;      
    uint8_t result = I2C_Write_EXIO(TCA9554_OUTPUT_REG,Data);  
    if (result != 0) {                         
      printf("Failed to set GPIO!!!\r\n");
    }
  }
  else                                           
    printf("Parameter error, please enter the correct parameter!\r\n");
}
void Set_EXIOS(uint8_t PinState)                          // Set 7 pins to the PinState state such as :PinState=0x23, 0010 0011 state (the highest bit is not used)
{
  uint8_t result = I2C_Write_EXIO(TCA9554_OUTPUT_REG,PinState); 
  if (result != 0) {                  
    printf("Failed to set GPIO!!!\r\n");
  }
}
/********************************************************** Flip EXIO state **********************************************************/  
void Set_Toggle(uint8_t Pin)                              // Flip the level of the TCA9554PWR Pin
{
    uint8_t bitsStatus = Read_EXIO(Pin);                 
    Set_EXIO(Pin,(bool)!bitsStatus); 
}
/********************************************************* TCA9554PWR Initializes the device ***********************************************************/  
void TCA9554PWR_Init(uint8_t PinState)                  // Set the seven pins to PinState state, for example :PinState=0x23, 0010 0011 State  (Output mode or input mode) 0= Output mode 1= Input mode. The default value is output mode
{                  
  Mode_EXIOS(PinState);      
}
