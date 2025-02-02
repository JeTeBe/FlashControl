//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"

// TFT start 
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

//#define USE_PIN // Uncomment this to use PIN during pairing. The pin is specified on the line below
const char *pin = "1234"; // Change this to more secure PIN.

String device_name = "FLASHCONTROL1";
int led = 0;
int interval = 0;
String sendStr = "";
uint8_t dimLevel = 75;
uint8_t oldDimLevel = 0;
uint8_t level = 75;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin(device_name); //Bluetooth device name
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());
  //Serial.printf("The device with name \"%s\" and MAC address %s is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str(), SerialBT.getMacString()); // Use this after the MAC method is implemented
  #ifdef USE_PIN
    SerialBT.setPin(pin);
    Serial.println("Using PIN");
  #endif

  tft.init();
  tft.setRotation(1);
  tft.setTextSize(3);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);  // Adding a black background colour erases previous text automatically
  tft.setCursor( 0, 0);
  tft.print(String(device_name));
  tft.setTextSize(7);
}

char* ConvertStringToCharArray(String S)
{
  int   ArrayLength  =S.length()+1;    //The +1 is for the 0x00h Terminator
  char  CharArray[ArrayLength];
  S.toCharArray(CharArray,ArrayLength);

  return(CharArray);
}

void loop() {
  if (interval == 30) {
    interval = 0;
    if (led == 0) {
      led = 1;
    } else {
      led = 0;
    }
    uint8_t byteArray[6] =  "led ";
    byteArray[4] = 0x30 + led; // '0' or '1'
    byteArray[5] = 0x0A; //LF
    if (SerialBT.available()) {
      SerialBT.write(byteArray, 6);
      Serial.printf("LED %i\r",led);
    }
  } else {
    interval = interval + 1;
  }

  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    uint8_t receivedData = SerialBT.read();
    ParseData(receivedData);
  }
  delay(20);
  if (oldDimLevel != dimLevel)
  {
    // Clear oldDimLevel
    tft.setTextColor(TFT_BLUE, TFT_BLUE);
    tft.setCursor( 50, 50 );
    tft.print(String(oldDimLevel));
    // Set dimLevel
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setCursor( 50, 50 );
    tft.print(String(dimLevel));
    oldDimLevel = dimLevel;
  }
  //tft.setTextColor(TFT_WHITE, TFT_BLUE);  // Adding a black background colour erases previous text automatically
  //tft.setCursor( 50, 50 );
  //tft.print(String(dimLevel));
//  int textSize = 3;
//  if ((dimLevel>=10) & (dimLevel<99)) { textSize = 2; }
//  if (dimLevel<10) { textSize = 1; }
  
//  tft.setCursor( 50+textSize*7, 50 );
//  tft.setTextColor(TFT_RED, TFT_RED);  // Adding a black background colour erases previous text automatically
//  tft.print("111");
  
  uint8_t ucArray[6] =  "2 000";
  uint8_t hund = (dimLevel / 100);
  level = dimLevel - hund * 100;
  uint8_t dec = (level / 10);
  level = level - dec * 10;
  uint8_t units = level;
  Serial.printf("dimLevel: %i = h: %i, d: %i, u: %i\r\n", dimLevel, hund, dec, units);
  ucArray[2] = 0x30 + hund; 
  ucArray[3] = 0x30 + dec; 
  ucArray[4] = 0x30 + units; 
  ucArray[5] = 0x0A; //LF
  if (SerialBT.available()) {
    SerialBT.write(ucArray, 6);
  }
}

const int WAIT_FOR_SYNC = 1;
const int GET_ID        = 2;
const int GET_VALUE     = 3;

int state = WAIT_FOR_SYNC;
uint8_t ID = 0;
uint8_t VALUE = 0;
void ParseData(uint8_t inval)
{
  switch(state)
  {
    case WAIT_FOR_SYNC:
      if (inval == 10) // LF end of message
      {
        state = GET_ID;
      }
      break;
    case GET_ID:
      if (inval == 32) // space end of ID
      {
        state = GET_VALUE;
      }
      else
      {
        ID = ID * 10 + (inval-48);  // Correct from ascii value
      }
      break;  
    case GET_VALUE:
      if (inval == 10) // LF end of value, start of next ID
      {
        state = GET_ID;
        if (VALUE != dimLevel)
        {
          oldDimLevel = dimLevel;
          dimLevel = VALUE;
        }
        // Reset for next round
        VALUE = 0;
        ID = 0;
      }
      else
      {
        VALUE = VALUE * 10 + (inval-48);  // Correct from ascii value
      }
      break;  
  }
}
