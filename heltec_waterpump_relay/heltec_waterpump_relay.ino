#include "LoRaWan_APP.h"
#define PUMP_PIN 7
  
/* OTAA para*/ // this is for heltec-oled-7
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD8, 0x00, 0x42, 0x44 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x3D, 0xD2, 0x71, 0xD2, 0xC8, 0xC5, 0xE4, 0x8E, 0x13, 0x12, 0x35, 0xB1, 0xDF, 0x41, 0xD6, 0xBE };

/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };  /*LoraWan channelsmask, default channels 0-7*/ 
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_US915; /*LoraWan region, select in arduino IDE tools*/
DeviceClass_t  loraWanClass = CLASS_A; /*LoraWan Class, Class A and Class C are supported*/
uint32_t appTxDutyCycle = 60000;  /*the application data transmission duty cycle.  value in [ms].*/
bool overTheAirActivation = true; /*OTAA or ABP*/
bool loraWanAdr = true;  /*ADR enable*/
bool isTxConfirmed = true;  /* Indicates if the node is sending confirmed or unconfirmed messages */

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

unsigned long pumpStartTime = 0;  // Stores when the pump was turned on
bool pumpRunning = false;         // Tracks whether the pump is running

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
    // Uplink the LED state (0x00 = OFF, 0x01 = ON)
    appDataSize = 1; // payload size of 1 byte
    appData[0] = (pumpRunning) ? 0x01 : 0x00;  // 0x01 for ON, 0x00 for OFF
    LoRaWAN.send(); // send uplink data
}

//downlink data handle function example
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
    Serial.printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n", 
        mcpsIndication->RxSlot ? "RXWIN2" : "RXWIN1", 
        mcpsIndication->BufferSize, 
        mcpsIndication->Port);
    
    Serial.print("+REV DATA:");
    for (uint8_t i = 0; i < mcpsIndication->BufferSize; i++) {
        Serial.printf("%02X", mcpsIndication->Buffer[i]);
    }
    Serial.println();

    // Check if downlink data is received
    if (mcpsIndication->BufferSize > 0) {
        if (mcpsIndication->Buffer[0] == 0x01) {  
            Serial.println("Turning ON pump/LED for 15 seconds");
            digitalWrite(LED_BUILTIN, HIGH);  // Turn on LED
            digitalWrite(PUMP_PIN, HIGH);     // Turn on pump

            pumpStartTime = millis();  // Record start time
            pumpRunning = true;  // Set flag
        } 
        else if (mcpsIndication->Buffer[0] == 0x00) 
        {
            // Turn OFF pump and LED if 0x00 is received
            digitalWrite(LED_BUILTIN, LOW);
            digitalWrite(PUMP_PIN, LOW);
            pumpRunning = false;  // Reset pump state
            Serial.println("Turning OFF pump/LED immediately");
        }
    }
}

void checkPumpTimer()
{
  if (pumpRunning && (millis() - pumpStartTime >= 15000))  // 15 seconds elapsed?
  {
    digitalWrite(LED_BUILTIN, LOW);  // Turn LED OFF
    digitalWrite(PUMP_PIN, LOW);     // Turn off pump
    pumpRunning = false;   // Reset state
    Serial.println("Pump OFF (Auto shutdown after 15 s.)");
  }
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
  pinMode(LED_BUILTIN, OUTPUT);   // initialize LED 
  digitalWrite(LED_BUILTIN, LOW); // ensure the LED is off upon setup run
  pinMode(PUMP_PIN, OUTPUT);  // initialize pump pin
  digitalWrite(PUMP_PIN, LOW); // ensure the pump is off upon setup run
}

void loop() {
    switch (deviceState) {
        case DEVICE_STATE_INIT:
            LoRaWAN.init(loraWanClass, loraWanRegion);
            LoRaWAN.setDefaultDR(3);
            break;

        case DEVICE_STATE_JOIN:
            LoRaWAN.join();
            break;

        case DEVICE_STATE_SEND:
            prepareTxFrame(appPort);
            LoRaWAN.send();
            deviceState = DEVICE_STATE_CYCLE;
            break;

        case DEVICE_STATE_CYCLE:
            txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
            LoRaWAN.cycle(txDutyCycleTime);
            deviceState = DEVICE_STATE_SLEEP;
            break;

        case DEVICE_STATE_SLEEP:  // device enters sleep mode
            if (!pumpRunning)
            {
              LoRaWAN.sleep(loraWanClass);
            }
            break;

        default:
            deviceState = DEVICE_STATE_INIT;
            break;
    }
    checkPumpTimer();
}