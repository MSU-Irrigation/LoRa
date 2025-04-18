#include "LoRaWan_APP.h"
#include <Wire.h> 
#include <Adafruit_SHT31.h>
#include <SdFat.h>
void downLinkDataHandle(McpsIndication_t *mcpsIndication);
#define PUMP_PIN 7; // pin on heltec used by pump for IN

/* Over the Air Activation (OTAA) Parameters. Get each node's parameters off TTS. */ 
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD8, 0x00, 0x42, 0x44 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x3D, 0xD2, 0x71, 0xD2, 0xC8, 0xC5, 0xE4, 0x8E, 0x13, 0x12, 0x35, 0xB1, 0xDF, 0x41, 0xD6, 0xBE };

/* ABP para*. (not used since we're using OTAA but the compiler doesn't like if edited it out)*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67 };
uint32_t devAddr = (uint32_t)0x007e6ae1;

/*LoraWan channelsmask           0xFF00 for US915. Leave the rest of these the same.*/
uint16_t userChannelsMask[6] = { 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/* LoRaWAN Configuration */
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_US915;  // select US915 from tool tab if ACTIVE_REGION in Arduino IDE or type LORAMAC_REGION_US915 in here.
DeviceClass_t loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 900000;  // this is the time interval between data being sent. Currently set for 15 minutes. // FIXME set interval
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t confirmedNbTrials = 4;
uint8_t appPort = 2;

/* Prepares LoRaWAN payload */
// Use the function to format the data being sent
static void prepareTxFrame(uint8_t port) 
{
    // This sends 03 
    appDataSize = 1;
    appData[0] = 0x03;
}

void setup() 
{
    Serial.begin(115200);
    pinMode(PUMP_PIN, OUTPUT);

    // Initialize LoRaWAN
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
    LoRaWAN.init(loraWanClass, loraWanRegion);
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
}

void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
    Serial.print("Received downlink on port ");
    Serial.println(mcpsIndication->Port);

    // Check if Buffer is NULL before accessing it
    if (mcpsIndication->Buffer == NULL || mcpsIndication->BufferSize == 0)
    {
        Serial.println("Received empty downlink.");
        return;
    }

    Serial.print("Payload: ");
    for (uint8_t i = 0; i < mcpsIndication->BufferSize; i++)
    {
        Serial.print(mcpsIndication->Buffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    uint8_t command = mcpsIndication->Buffer[0];  // Read first byte of downlink

    if (command == 0x01) 
    {
        digitalWrite(LED_BUILTIN, HIGH);  // Turn LED on
        Serial.println("LED ON");
    } 
    else if (command == 0x00) 
    {
        digitalWrite(LED_BUILTIN, LOW);  // Turn LED off
        Serial.println("LED OFF");
    }
}
