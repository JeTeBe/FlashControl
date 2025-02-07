/////////////////////////////////////////////////////////////////
/*
  Digipot.cpp - Arduino Library to simplify working with digital
  potentiometers
  Created by Jos Beukeveld, Februari 4, 2025.
*/
/////////////////////////////////////////////////////////////////

#include "Arduino.h"
#include "DigiPot.h"

/////////////////////////////////////////////////////////////////

DigiPot::DigiPot(byte pinChipSelect, byte pinUpDown, byte pinIncrement) 
{
  pinCS = pinChipSelect;
  pinINC = pinUpDown;
  pinUD = pinIncrement;
  pinMode(pinCS, OUTPUT);
  digitalWrite(pinCS, HIGH);
  pinMode(pinINC, OUTPUT);
  digitalWrite(pinINC, HIGH);
  pinMode(pinUD, OUTPUT);
  digitalWrite(pinUD, LOW);
  StoredValue = NO_VALUE_SET;
}

/////////////////////////////////////////////////////////////////
  
void DigiPot::setStoredValue(uint8_t value) 
{
  StoredValue = value;  // This is the starting point.
                        // The digital potmeter will restore its
                        //  latest value
}
      
/////////////////////////////////////////////////////////////////

void DigiPot::setValue(uint8_t value) 
{

  if (StoredValue == NO_VALUE_SET)
  {
    // No value set first calibrate
    doCalibrate();
    Serial.println("CALIBRATING");
  }
  // Now we are in the position to goto the new value.
  if (value == StoredValue)
  {
    // Nothing to do
    return;
  }
  // We are good to go, enable chip
  digitalWrite(pinCS, LOW);
  delay(1);
  
  // Do we have to go up or down.
  if (value < StoredValue)
  {
    // We have to go up.
    digitalWrite(pinUD, HIGH);
    delay(1);
  } else {
    // We have to go down.
    digitalWrite(pinUD, LOW);
    delay(1);
  }
  for (int i = 0; i < (abs(value - StoredValue)); i++)
  {
    digitalWrite(pinINC, LOW);
    delay(1);
    digitalWrite(pinINC, HIGH);    
    delay(1);
  }
  // We are done, disable chip
  digitalWrite(pinCS, HIGH);
  delay(1);
  StoredValue = value;
}
    
/////////////////////////////////////////////////////////////////

void DigiPot::doCalibrate() 
{
  // After calibration the level is 0

  // Calibating code....
  // We are good to go, enable chip
  digitalWrite(pinCS, LOW);
  delay(1);
  // We have to go down.
  digitalWrite(pinUD, LOW);
  delay(1);
  // Set to 0
  for (int i = 0; i < MAX_STEPS; i++)
  {
    digitalWrite(pinINC, LOW);
    delay(1);
    digitalWrite(pinINC, HIGH);    
    delay(1);
  }
  // We are done, disable chip
  digitalWrite(pinCS, HIGH);
  delay(1);
  StoredValue = 0;
}
    
/////////////////////////////////////////////////////////////////
