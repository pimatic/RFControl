#include "Arduino.h"

#if (defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)) && !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 400   //In combination with Homeduino maximum 490*2Byte are possible. Higher values block the arduino
#endif
#if (defined(__AVR_ATmega32U4__) || defined(TEENSY20) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || defined(ESP8266)) && !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 512   //on the bigger arduino we have enough SRAM
#endif
#if !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 255   // fallback for undefined Processor.
#endif

static inline void hw_attachInterrupt(uint32_t pin, void (*callback)(void)) {
  attachInterrupt(pin, callback, CHANGE);
}

static inline void hw_detachInterrupt(uint32_t pin) {
  detachInterrupt(pin);
}

static inline void hw_delayMicroseconds(unsigned long time_to_wait) {
  //  delayMicroseconds() only works up to 16383 micros
  // https://github.com/pimatic/rfcontroljs/issues/29#issuecomment-85460916
  while(time_to_wait > 16000) {
    delayMicroseconds(16000);
    time_to_wait -= 16000;
  }
  delayMicroseconds(time_to_wait);
}

static inline void hw_pinMode(uint8_t pin, uint8_t mode) {
  pinMode(pin, mode);
}

static inline void hw_digitalWrite(uint8_t pin, uint8_t value) {
  digitalWrite(pin, value);
}

static inline unsigned long hw_micros() {
  return micros();
}
