#include "Arduino.h"

#if (defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)) && !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 400   //In combination with Homeduino maximum 490*2Byte are possible. Higher values block the arduino
#endif
#if (defined(__AVR_ATmega32U4__) || defined(TEENSY20) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || defined(ESP8266)) && !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 512   //on the bigger arduino we have enough SRAM
#endif
#if defined(ESP8266)
  #define MAX_RECORDINGS 512  // ESP8266 has far enough ram
#endif
#if !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 255   // fallback for undefined Processor.
#endif

static inline void hw_attachInterrupt(int pin, void (*callback)(void)) {
  pinMode(pin, INPUT);
  attachInterrupt(pin, callback, CHANGE);
}

static inline void hw_detachInterrupt(int pin) {
  detachInterrupt(pin);
}

static inline void hw_delayMicroseconds(uint32_t time_to_wait) {
  //  delayMicroseconds() only works up to 16383 micros
  // https://github.com/pimatic/rfcontroljs/issues/29#issuecomment-85460916
  while(time_to_wait > 16000) {
    #if defined(ESP8266)
      delay(16)
    #else
      delayMicroseconds(16000);
    #endif
    time_to_wait -= 16000;
  }
  delayMicroseconds(time_to_wait);
}

static inline void hw_pinMode(int pin, int mode) {
  pinMode(pin, mode);
}

static inline void hw_digitalWrite(int pin, int value) {
  digitalWrite(pin, value);
}

static inline uint32_t hw_micros() {
  return micros();
}
