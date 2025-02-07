//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include "EEPROM.h"
#include "Button2.h"
#include "DigiPot.h"
#include "BluetoothSerial.h"

// TFT start 
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h


// the current address in the EEPROM (i.e. which byte
// we're going to write to next)
int addr = 0;
#define EEPROM_SIZE 1
#define BUTTON_1            35
#define BUTTON_2            0
//#define USE_PIN // Uncomment this to use PIN during pairing. The pin is specified on the line below
const char *pin = "1234"; // Change this to more secure PIN.

const int WAIT_FOR_SYNC = 1;
const int GET_ID        = 2;
const int GET_VALUE     = 3;

const int EE_LAST_DIM_IDX = 0;

int state = WAIT_FOR_SYNC;
uint8_t ID = 0;
uint8_t VALUE = 0;
String device_name = "FLASHCONTROL1";
int led = 0;
int interval = 0;
String sendStr = "";
uint8_t dimLevel = 75;
uint8_t oldDimLevel = 0;
uint8_t level = 75;
int firstTimeBT = 0;
bool stopUpdateRemoteDimlevel = false;
bool plusShown = false;

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

DigiPot potmeter(25,26,27);

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);


  // Eeprom
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  Serial.println(" bytes read from Flash . Values are:");
  dimLevel = byte(EEPROM.read(EE_LAST_DIM_IDX));
  //potmeter.doCalibrate();
  potmeter.setValue(dimLevel);
  Serial.println("Stored dimlevel: " + String(dimLevel)+ "\r\n");
 
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
  tft.setCursor( 1, 1);
  tft.print(String(device_name));

  button_init();
  showConnectionStatus();
}

char* ConvertStringToCharArray(String S)
{
  int   ArrayLength  =S.length()+1;    //The +1 is for the 0x00h Terminator
  char  CharArray[ArrayLength];
  S.toCharArray(CharArray,ArrayLength);

  return(CharArray);
}

void loop() {
  if (interval == 50) {
    interval = 0;
    if (led == 0) {
      led = 1;
    } else {
      led = 0;
    }
    //Serial.println("LED " + String(led));
    uint8_t byteArray[6] =  "led 0";
    byteArray[4] = 0x30 + led; // '0' or '1'
    byteArray[5] = 0x0A; //LF
    SerialBT.write(byteArray, 6);
    if (SerialBT.connected()) {
      if (firstTimeBT == 0)
      {
        SendInitialDimLevel();
        Serial.println("Send Initial");
        firstTimeBT = 1;
      }
    } else {
      firstTimeBT = 0;
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
    tft.setTextSize(7);
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
  
  uint8_t ucArray[6] =  "2 000";
  fillDimLevel( dimLevel, &(ucArray[2]));
  ucArray[5] = 0x0A; //LF
  if (SerialBT.available()) {
    SerialBT.write(ucArray, 6);
  }
  button_loop();
}

void SendInitialDimLevel()
{
  uint8_t firstByteArray[6] =  "1 000";
  fillDimLevel( dimLevel, &(firstByteArray[2]));
  firstByteArray[5] = 0x0A; //LF
  SerialBT.write(firstByteArray, 6);
}

void fillDimLevel( uint8_t value, uint8_t* arr)
{
  uint8_t level;
  uint8_t hund = (value / 100);
  level = value - hund * 100;
  uint8_t dec = (level / 10);
  level = level - dec * 10;
  uint8_t units = level;
  //Serial.printf("dimLevel: %i = h: %i, d: %i, u: %i\r\n", dimLevel, hund, dec, units);
  arr[0] = 0x30 + hund; 
  arr[1] = 0x30 + dec; 
  arr[2] = 0x30 + units; 
}

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
          if (stopUpdateRemoteDimlevel == true){
            // Check if we see the command dimLevel back
            if (dimLevel == VALUE)
            {
              stopUpdateRemoteDimlevel = false;
            }
          }else{
            dimLevel = VALUE;
            potmeter.setValue(dimLevel);
          }
          Serial.println("Writing " + String(dimLevel) + " to EEPROM");
          EEPROM.write(EE_LAST_DIM_IDX,dimLevel);
          EEPROM.commit();
        }
        // Reset for next round
        VALUE = 0;
        ID = 0;
        showConnectionStatus();
      }
      else
      {
        VALUE = VALUE * 10 + (inval-48);  // Correct from ascii value
      }
      break;  
  }
}

void showConnectionStatus()
{
  tft.setTextSize(3);
  // Notify GUI of data received
  // Clear oldDimLevel
  tft.setTextColor(TFT_BLUE, TFT_BLUE);
  tft.setCursor( 200, 60 );
  if (plusShown == true) {
    tft.fillCircle(205, 72, 20, TFT_DARKGREEN);
  } else {
    tft.fillCircle(205, 72, 20, TFT_GREEN );
  }
  plusShown = !plusShown;
}

void CalibratePotmeter()
{
  Serial.print("Calibrating digital potentiometer ....");
  Serial.println("Done.");
}

void button_init()
{
    btn1.setLongClickHandler([](Button2 & b) {
      CalibratePotmeter();
    });
    btn1.setClickHandler([](Button2 & b) {
      stopUpdateRemoteDimlevel = true;
      if (dimLevel < 100) {
        dimLevel = dimLevel + 1;
      }else{
        dimLevel = 100;
      }
      EEPROM.write(EE_LAST_DIM_IDX,dimLevel);
      EEPROM.commit();
      SendInitialDimLevel();
      potmeter.setValue(dimLevel);
    });
    btn1.setDoubleClickHandler([](Button2 & b) {
      stopUpdateRemoteDimlevel = true;
      if (dimLevel < 91) {
        dimLevel = dimLevel + 10;
      }else{
        dimLevel = 100;
      }
      EEPROM.write(EE_LAST_DIM_IDX,dimLevel);
      EEPROM.commit();
      SendInitialDimLevel();
      potmeter.setValue(dimLevel);
    });

    btn2.setClickHandler([](Button2 & b) {
      stopUpdateRemoteDimlevel = true;
      if (dimLevel > 1) {
        dimLevel = dimLevel - 1;
      } else {
        dimLevel = 1;
      }
      EEPROM.write(EE_LAST_DIM_IDX,dimLevel);
      EEPROM.commit();
      SendInitialDimLevel();
      potmeter.setValue(dimLevel);
    });
    
    btn2.setDoubleClickHandler([](Button2 & b) {
      stopUpdateRemoteDimlevel = true;
      if (dimLevel > 10) {
        dimLevel = dimLevel - 10;
      } else {
        dimLevel = 1;
      }
      EEPROM.write(EE_LAST_DIM_IDX,dimLevel);
      EEPROM.commit();
      SendInitialDimLevel();
      potmeter.setValue(dimLevel);
    });
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}
