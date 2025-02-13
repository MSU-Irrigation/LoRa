/*
  Name: heltec_lora_with_scd30.ino
  Author: Tommy Rose
  Created on: 02/07/2025

  Purpose: Reads from the SCD30 sensor multiple times at startup, averages the readings, and sends the data via LoRa every 5 minutes.

  Version 1.2 
*/

#include "LoRaWan_APP.h"
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <adafruit-sht31.h>
#include <SdFat.h>


#define LW_PIN 7  // access gpio 7 for leaf wetness analog input.


/* Over the Air Activation (OTAA) Parameters */       // testing with heltec-oled-3
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD8, 0x00, 0x3F, 0x57 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x02, 0x2A, 0xE1, 0x13, 0xB7, 0x4C, 0x88, 0xB3, 0xCA, 0x31, 0x03, 0x28, 0x38, 0x3D, 0x8B, 0x0F };

/* ABP para*. (not used since we're using OTAA but the compiler will complain if I edit it out)*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67 };
uint32_t devAddr = (uint32_t)0x007e6ae1;

/*LoraWan channelsmask*/
uint16_t userChannelsMask[6] = { 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/* LoRaWAN Configuration */
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;  // US915
DeviceClass_t loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 300000;  // 5 minutes
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t confirmedNbTrials = 4;
uint8_t appPort = 2;

/* SCD30 Sensor */
SCD30 scd30Sensor;
uint16_t co2 = 0;
float temp = 0.0;
float humid = 0.0;

/*SHT31 Sensor */
Adafruit_SHT31 sht31 = Adafruit_SHT31();
float I2C_t1 = 0.00;
float I2C_h1 = 0.00;

/* Leaf Wetness Sensor */
float leafWetness {0.0};

/* rain */
#define RAIN_PIN 2          // interrupt pin
#define CALC_INTERVAL 1000  // increment of measurements
#define DEBOUNCE_TIME 15    // time * 1000 in microseconds required to get through bounce noise

volatile unsigned int rainTrigger = 0;
volatile unsigned long last_micros_rg;


/* Prepares LoRaWAN payload */
static void prepareTxFrame(uint8_t port) {
  appDataSize = 6;
  appData[0] = (co2 >> 8) & 0xFF;
  appData[1] = co2 & 0xFF;
  int16_t tempScaled = (int16_t)(temp * 100);
  appData[2] = (tempScaled >> 8) & 0xFF;
  appData[3] = tempScaled & 0xFF;
  int16_t humidScaled = (int16_t)(humid * 100);
  appData[4] = (humidScaled >> 8) & 0xFF;
  appData[5] = humidScaled & 0xFF; 
}




void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  Wire.begin();
  LoRaWAN.displayMcuInit();

  // Initialize SCD30 Sensor
  Serial.println(F("Initializing SCD30..."));
  if (!scd30Sensor.begin()) {
    Serial.println("SCD30 not detected! Check wiring.");
    while (1)
      ;
  }
  Serial.println("Taking 3 readings and averaging...");
  uint16_t co2Sum = 0;
  float tempSum = 0.0;
  float humidSum = 0.0;
  float lwSum = 0.0;
  const int numReadings = 3;
  float I2C_t1Sum = 0.0;
  float I2C_h1Sum = 0.0;

  // Get multiple readings for averaging and more accurate results.
  for (int i = 0; i < numReadings; i++) {
    delay(2000);  // Wait 2 seconds between readings
    if (scd30Sensor.readMeasurement()) {
      co2Sum += scd30Sensor.getCO2();
      tempSum += scd30Sensor.getTemperature();
      humidSum += scd30Sensor.getHumidity();
      lwSum += analogRead(LW_PIN);
      I2C_t1Sum += sht31.readTemperature();
      I2C_h1Sum += sht31.readHumidity();
    } 
    else 
    {
      Serial.println("Failed to read sensor!");
    }
  }


  // Average the readings
  co2 = co2Sum / numReadings;
  temp = tempSum / numReadings;
  humid = humidSum / numReadings;
  leafWetness = lwSum / numReadings;
  I2C_t1 = I2C1_tSum / numReadings;
  I2C_h1 = I2C1_hSum / numReadings;
  rainTrigger_cal = rainTrigger * 0.01;


  Serial.printf("Averaged CO2: %u ppm, Temp: %.1f C, Humidity: %.1f %%, Leaf Wetness: %.1f \n", co2, TempSHT: %.1f C, HumiditySHT: %.1f %%, Rain: %.1f inch, temp, humid, leafWetness, I2C_t1, I2C_h1, rainTrigger_cal);


}



  switch (deviceState) {
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



void loop() {


 for (int i = 0; i < numReadings; i++) {
    delay(2000);  // Wait 2 seconds between readings
    if (scd30Sensor.readMeasurement()) {
      co2Sum += scd30Sensor.getCO2();
      tempSum += scd30Sensor.getTemperature();
      humidSum += scd30Sensor.getHumidity();
      lwSum += analogRead(LW_PIN);
      I2C_t1Sum += sht31.readTemperature();
      I2C_h1Sum += sht31.readHumidity();
  

    } 
    else 

    {
      Serial.println("Failed to read sensor!");
    }
  }


  // Average the readings
  co2 = co2Sum / numReadings;
  temp = tempSum / numReadings;
  humid = humidSum / numReadings;
  leafWetness = lwSum / numReadings;
  I2C_t1 = I2C1_tSum / numReadings;
  I2C_h1 = I2C1_hSum / numReadings;

  rainTrigger_cal = rainTrigger * 0.01;
  Serial.printf("Averaged CO2: %u ppm, Temp: %.1f C, Humidity: %.1f %%, Leaf Wetness: %.1f \n", co2, TempSHT: %.1f C, HumiditySHT: %.1f %%, Rain: %.1f inch, temp, humid, leafWetness, I2C_t1, I2C_h1, rainTrigger_cal);

}


  switch (deviceState) {
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

  delay(180000);


}


void countingRain() {

  if((long)(micros() - last_micros_rg) >= DEBOUNCE_TIME * 1000) { 
   rainTrigger += 1;
   last_micros_rg = micros();
  }  
}