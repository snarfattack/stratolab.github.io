#include "HT_st7735.h"
#include <RadioLib.h>

// Heltec Tracker v1.1/1.2 LoRa Pins
#define LORA_NSS  8
#define LORA_DIO1 14
#define LORA_NRST 12
#define LORA_BUSY 13
#define VEXT_CTRL 3  // GPIO 3 powers LoRa and Screen on Tracker v1.2

HT_st7735 screen;
SX1262 lora = new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY);

volatile bool packetAvailable = false;

// Interrupt function
void onReceive(void) {
  packetAvailable = true;
}

void setup() {
  Serial.begin(115200);

  // 1. Power on the onboard LoRa and Screen
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW); 
  delay(100);

  // 2. Initialize Screen
  screen.st7735_init();
  screen.st7735_fill_screen(ST7735_BLACK);
  screen.st7735_write_str(0, 0, "BALLOON RECV", Font_7x10, ST7735_YELLOW, ST7735_BLACK);
  
  // 3. Initialize LoRa 
  // MUST MATCH TRANSMITTER EXACTLY:
  // freq: 915.0, bw: 125.0, sf: 7, cr: 5, sync: 0x12, pwr: 10, preamble: 8, tcxo: 1.6V, LDO: false (DC-DC)
  int status = lora.begin(915.0, 125.0, 7, 5, 0x12, 10, 8, 1.6, false); 

  if (status == RADIOLIB_ERR_NONE) {
    screen.st7735_write_str(0, 15, "LORA: OK (915MHz)", Font_7x10, ST7735_GREEN, ST7735_BLACK);
    lora.setPacketReceivedAction(onReceive);
    lora.startReceive(); 
  } else {
    screen.st7735_write_str(0, 15, "LORA: FAIL", Font_7x10, ST7735_RED, ST7735_BLACK);
    screen.st7735_write_str(0, 25, "ERR: " + String(status), Font_7x10, ST7735_WHITE, ST7735_BLACK);
  }
}

void loop() { 
  if(packetAvailable){
    readPacket();
  } 
}

void readPacket() {
    String data;
    int status = lora.readData(data);
    
    if (status == RADIOLIB_ERR_NONE) {
        // Clear a portion of the screen for new data
        screen.st7735_fill_screen(ST7735_BLACK);
        screen.st7735_write_str(0, 0, "RECV SUCCESS:", Font_7x10, ST7735_YELLOW, ST7735_BLACK);
        
        // Display the payload (Time, Lat, Lon, Alt, Sats)
        screen.st7735_write_str(0, 20, data, Font_7x10, ST7735_WHITE, ST7735_BLACK);

        // Signal Quality Stats
        String stats = "R:" + String(lora.getRSSI()) + " S:" + String(lora.getSNR());
        screen.st7735_write_str(0, 110, stats, Font_7x10, ST7735_CYAN, ST7735_BLACK);

    } else {
        screen.st7735_write_str(0, 50, "RX Error: " + String(status), Font_7x10, ST7735_RED, ST7735_BLACK);
    }

    packetAvailable = false;
    lora.startReceive(); // Re-enable receive mode
}
