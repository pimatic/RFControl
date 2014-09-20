#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <cstdio>

#define HIGH 0x1
#define LOW  0x0
#define INPUT 0x0
#define OUTPUT 0x1

#define CHANGE 1	
#define FALLING 2
#define RISING 3

void pinMode(uint8_t, uint8_t);
void digitalWrite(uint8_t, uint8_t);
int digitalRead(uint8_t);
int analogRead(uint8_t);
void analogReference(uint8_t mode);
void analogWrite(uint8_t, int);
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long);
void delayMicroseconds(unsigned int us);
void attachInterrupt(uint8_t, void (*)(void), int mode);
void detachInterrupt(uint8_t);

static char sate2string[4][255] = {
	"STATUS_WAITING",
	"STATUS_RECORDING",
	"STATUS_VERIFY",
	"STATUS_DATA_READY"
};