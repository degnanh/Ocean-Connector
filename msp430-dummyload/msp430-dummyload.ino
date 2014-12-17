/* 
 *  MSP430 Matrix dummyload
 *  Intended for use as a configurable load over serial.
 *  Written in Energia 0101E0014, unforunately not built in CCS.
 *  
 *  Version: 2.0
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
 *  2.0  added Conway's game of life logic
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
uint8_t life[LED_ROWS][LED_ROWS];

/* timer variable LED row updating, units in microseconds */
unsigned long LEDLastRowTime = 0;
const unsigned long LEDRowinterval = 750;

/* screen update date, units in milliseconds */
unsigned long LEDchangeTime = 0;
const unsigned long LEDchangeInterval = 175;


/* function prototypes */
void updateLEDs();
void updateLEDRow();
uint8_t countNeighbours();

/* Initialize digital pin output modes */
void setup()
{
  /* Fill the array with random values */
  randomSeed(2);
  for (uint8_t i = 0; i < LED_ROWS; i++) {
    pinMode(LED[i], OUTPUT);
    for (uint8_t j=0; j < LED_ROWS; j++) {
      life[j][i] = random(255) % 2;
    }
  }

  /* Initialize SPI */
  pinMode(SPI_SSpin, OUTPUT);
  pinMode(SPI_LEpin, OUTPUT);
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  Serial.begin(9600);
}


/* main loop has two asynchronous events - drawing each row, 
 *   and updating the displayed content.
 */
void loop() {
  /* change the displayed LED pattern */
  if (millis() - LEDchangeTime >= LEDchangeInterval) {
    Serial.write(life[1][2]+65);
    Serial.write(life[3][2]+65);
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
  for (int x=0; x < LED_ROWS; x++) {
    red[x] = 0x00;
    green[x] = 0x00;
    for (int y=0; y < LED_ROWS; y++) {
      int n = countNeighbours(x,y);
      /* game logic: live cell stays alive with 2 or 3 neighbours */
      if (life[x][y]) {
        if (n <= 1 || n >= 4) {
          life[x][y] = 0;
          red[x] |= 0x01 << y;
        } 
        else {
          green[x] |= 0x01 << y;
          red[x] |= 0x01 << y;
        }
      } 
      else {
        /* if cell is dead, check to see if it becomes alive */
        if (n == 3) {
          life[x][y] = 1;
          green[x] |= 0x01 << y;
          
        }
      }
    }
  }
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


int countNeighbours(int x, int y)
{
  int count = 0;
  const int r = LED_ROWS;
  count += life[(x+1) % r][y];
  count += life[(x+1) % r][(y+1) % r];
  count += life[(x+1) % r][(y+r-1) % r];

  count += life[x][(y+1) % r];
  count += life[x][(y+r-1) % r];

  count += life[(x+r-1) % r][y];
  count += life[(x+r-1) % r][(y+1) % r];
  count += life[(x+r-1) % r][(y+r-1) % r];
  return count;
}



