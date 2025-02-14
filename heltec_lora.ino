/*
  Name: heltec_lora.ino
  Author: Tommy Rose
  Created on: 02/07/2025

  Purpose: Reads from the SHT31, leaf wetness, and rain gauge(?) sensors 3 times at startup, averages the readings, and sends the data via LoRa every 3 minutes.

  Version 1.3
*/

#include "LoRaWan_APP.h"
#include <Wire.h> 
#include <Adafruit_SHT31.h>
#include <SdFat.h>

/* Over the Air Activation (OTAA) Parameters */       // testing with heltec-oled-4
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD8, 0x00, 0x3F, 0x5C };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x48, 0x21, 0x5C, 0xBA, 0x93, 0xD7, 0xC8, 0x9B, 0x69, 0x9F, 0x4B, 0x78, 0xBD, 0xC1, 0x67, 0x11 };

/* ABP para*. (not used since we're using OTAA but the compiler will complain if you edit it out)*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67 };
uint32_t devAddr = (uint32_t)0x007e6ae1;

/*LoraWan channelsmask*/
uint16_t userChannelsMask[6] = { 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/* LoRaWAN Configuration */
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;  // select US915 from tool tab.
DeviceClass_t loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 180000;  // this is the time interval between data being sent
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t confirmedNbTrials = 4;
uint8_t appPort = 2;

/*SHT31 temp and humid sensor*/
Adafruit_SHT31 sht31 = Adafruit_SHT31();
float temp {0.0};
float humid {0.0};

/* Leaf Wetness Sensor */
#define LW_PIN 7  // access gpio 7 for leaf wetness analog input.
int leafWetness {0};

/* Rain Sensor */
#define RAIN_PIN 2          // interrupt pin
#define CALC_INTERVAL 1000  // increment of measurements
#define DEBOUNCE_TIME 15    // time * 1000 in microseconds required to get through bounce noise
volatile unsigned int rainTrigger = 0;
volatile unsigned long last_micros_rg;

/* Prepares LoRaWAN payload */
// Use the function to format the data being sent
static void prepareTxFrame(uint8_t port) 
{
  appDataSize = 6;
  appData[0] = (leafWetness >> 8) & 0xFF;
  appData[1] = leafWetness & 0xFF;
  int16_t tempScaled = (int16_t)(temp * 100);
  appData[2] = (tempScaled >> 8) & 0xFF;
  appData[3] = tempScaled & 0xFF;
  int16_t humidScaled = (int16_t)(humid * 100);
  appData[4] = (humidScaled >> 8) & 0xFF;
  appData[5] = humidScaled & 0xFF; 
}

void setup() 
{
  Serial.begin(115200);
  
  if(! sht31.begin(0x44))
  {
    Serial.println("Couldn't find SHT31");
    while (1) delay(100);
  }
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  Wire.begin();
  LoRaWAN.displayMcuInit();

  Serial.println("Taking 3 readings and averaging...");
  float tempSum = 0.0;
  float humidSum = 0.0;
  int lwSum = 0;

  const int numReadings = 3;

  // Get multiple readings for averaging and more accurate results.
  for (int i = 0; i < numReadings; i++) 
  {
    delay(2000);  // Wait 2 seconds between readings

    // Get the sensor readings from the sht31
    tempSum += sht31.readTemperature();
    humidSum += sht31.readHumidity();
    // Get the leaf wetness
    lwSum += analogRead(LW_PIN);
  }

  // Average the readings
  leafWetness = lwSum / numReadings;
  temp = tempSum / numReadings;
  humid = humidSum / numReadings;
  //rainTrigger_cal = rainTrigger * 0.01;

  Serial.printf("Temp: %.1f C, Humidity: %.1f %%, Leaf Wetness: %d, Rain: %.1f inch\n", temp, humid, leafWetness ); // add rainTrigger_cal once debugged
}

void loop() 
{
  switch (deviceState) 
  {
    case DEVICE_STATE_INIT:
      LoRaWAN.init(loraWanClass, loraWanRegion);
      LoRaWAN.setDefaultDR(2);
      break;
    case DEVICE_STATE_JOIN:
      LoRaWAN.displayJoining();
      LoRaWAN.join();
      break;
    case DEVICE_STATE_SEND:
      LoRaWAN.displaySending();
      prepareTxFrame(appPort);
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    case DEVICE_STATE_CYCLE:
      LoRaWAN.cycle(appTxDutyCycle);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    case DEVICE_STATE_SLEEP:
      LoRaWAN.displayAck();
      LoRaWAN.sleep(loraWanClass);
      break;
    default:
      deviceState = DEVICE_STATE_INIT;
      break;
  }
  rainTrigger = 0;
}

void countingRain() 
{
  if((long)(micros() - last_micros_rg) >= DEBOUNCE_TIME * 1000) 
  { 
   rainTrigger += 1;
   last_micros_rg = micros();
  }  
}
