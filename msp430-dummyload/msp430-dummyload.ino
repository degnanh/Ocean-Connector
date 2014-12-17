/* 
 *  MSP430 Matrix dummyload
 *  Intended for use as a configurable load over serial.
 *  Written in Energia 0101E0014, unforunately not built in CCS.
 *  
 *  Version: 1.0
 *
 *  Updates a bi-colour 8x8 LED matrix at about 1ms per row.
 *    With 8 rows, that is 125 Hz refresh rate. Datasheet allows
 *    the LED matrix to be driven at 90 mA per LED at PWM of
 *    1 kHz and 10% duty cycle (720 mA per row).
 *
 *    As-built circuit powers each row a max 500 mA because
 *    it is powered by USB!
 *
 *   Circuit details: 3V microcontroller digital outputs go to a
 *    darlington driver chip ULN2803, which triggers a 5V PNP into
 *    each row of LEDs (common anode). Each column's cathodes go
 *    to a constant-current LED sink driver TLC5916. These are
 *    daisy-chained, one for each colour of LED, and controlled
 *    via SPI.
 *
 *  Version history:
 *  1.0  Scroll a pattern vertically with no visible flicker
 */

/* SPI */
#include <SPI.h>
const int SPI_SSpin = P2_2;
const int SPI_LEpin = P2_4;

/* LED pin mapping */
#define LED_ROWS 8
const int LED[] = {
  P1_0, P1_3, P1_4, P2_0, P2_1, P2_3, P2_5, P2_2};

/* which LED row is currently HIGH */
uint8_t LEDCurrentRow = 0;

/* LED storage, global arrays */
uint8_t red[LED_ROWS];
uint8_t green[LED_ROWS];

/* timer variable LED row updating, units in microseconds */
unsigned long LEDLastRowTime = 0;
const unsigned long LEDRowinterval = 750;

/* screen update date, units in milliseconds */
unsigned long LEDchangeTime = 0;
const unsigned long LEDchangeInterval = 1000;


/* function prototypes */
void updateLEDs();
void updateLEDRow();

/* Initialize digital pin output modes */
void setup()
{
  /* Fill the array with random values */
  randomSeed(1);
  for (uint8_t i = 0; i < LED_ROWS; i++) {
    pinMode(LED[i], OUTPUT);
    red[i] = random(255);
    green[i] = random(255);
  }

  /* Initialize SPI */
  pinMode(SPI_SSpin, OUTPUT);
  pinMode(SPI_LEpin, OUTPUT);
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
}


/* main loop has two asynchronous events - drawing each row, 
 *   and updating the displayed content.
 */
void loop() {
  /* change the displayed LED pattern */
  if (millis() - LEDchangeTime >= LEDchangeInterval) {
    updateLEDs();
    LEDchangeTime = millis();
  }

  /* display the next row of LEDs */
  if (micros() - LEDLastRowTime >= LEDRowinterval) {
    updateLEDRow();
    LEDLastRowTime = micros();
  }
}


/* This scrolls the LED patterns vertically by one step */
void updateLEDs()
{
  uint8_t tr = red[0];
  uint8_t tg = green[0];
  for (int i=0; i<LED_ROWS-1; i++) {
    red[i] = red[i+1];
    green[i] = green[i+1];
  }
  red[7] = tr;
  green[7] = tg;
}

/* Turn off current LED row, feed in next rows over SPI, then turn on LEDs*/
void updateLEDRow()
{
  /* turn off the LED row, then increment */
  digitalWrite(LED[LEDCurrentRow],LOW);
  LEDCurrentRow++;
  if (LEDCurrentRow >= LED_ROWS) {
    LEDCurrentRow = 0;
  }

  /* turn off LED sink driver, then write SPI */
  digitalWrite(SPI_SSpin, HIGH);
  SPI.transfer(green[LEDCurrentRow]);
  SPI.transfer(red[LEDCurrentRow]);
  digitalWrite(SPI_LEpin, HIGH);
  digitalWrite(SPI_LEpin, LOW);

  /* turn on LED sink driver */
  digitalWrite(SPI_SSpin, LOW);

  /* turn on the LED row */
  digitalWrite(LED[LEDCurrentRow], HIGH);
}


