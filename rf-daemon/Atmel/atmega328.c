#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <EEPROM.h>

#define CONFIG_START 32

#define LCD_I2C_ADDR    0x20 // PCF8574T I2C Address

#define BACKLIGHT     3  
#define EN  2  
#define RW  1  
#define RS  0  
#define D4  4  
#define D5  5  
#define D6  6  
#define D7  7 

LiquidCrystal_I2C lcd(LCD_I2C_ADDR,EN,RW,RS,D4,D5,D6,D7);

RF24 radio(9,10);

const uint64_t pipes[2] = { 0xFAF0F0F0E1LL, 0xFAF0F0F0D2LL };

const int min_payload_size = 4;
const int max_payload_size = 32;
const int payload_size_increments_by = 1;

int next_payload_size = min_payload_size;

uint8_t len = 0;

byte maxa = 20;

char receive_payload[max_payload_size+1]; // +1 for a terminating NULL char

static char send_payload[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ789012";

void setup(void) {
    //maxa = EEPROM.read(CONFIG_START);
    //EEPROM.write(CONFIG_START, maxa);
    lcd.begin (20,4);
    delay(10);
    lcd.setBacklightPin(BACKLIGHT,POSITIVE);
    lcd.setBacklight(HIGH);
    lcd.clear();
    delay(10);
    lcd.home ();
    
    Serial.begin(57600);

    printf_begin();
    
    radio.begin();
    radio.setPALevel(RF24_PA_MAX); //RF24_PA_MIN = 0, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX, RF24_PA_ERROR 
    radio.setDataRate(RF24_250KBPS); //RF24_1MBPS = 0, RF24_2MBPS, RF24_250KBPS
    //radio.setAutoAck(1);
    radio.setRetries(15,15);
    
    radio.enableDynamicPayloads();
    
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);
    radio.startListening();
    radio.printDetails();
}

void loop(void) {
    radio.startListening();
    delay(10);
    if ( radio.available() ) {
      while ( radio.available() ) {
        len = radio.getDynamicPayloadSize();
        radio.read(receive_payload, len);
        receive_payload[len] = 0;
        printf("Payload size=%i value=%s\n\r", len, receive_payload);
        lcd.setCursor(0, 0);
        delay(1);
        lcd.print(receive_payload);
      }
    }
    radio.stopListening();
    delay(1);
    radio.write( send_payload, 16 );
    delay(10);
}
