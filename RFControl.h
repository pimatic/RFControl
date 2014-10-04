/*
  RFControl.h - 
*/
#ifndef ArduinoRf_h
#define ArduinoRf_h

#ifndef RF_CONTROL_SIMULATE_ARDUINO
#include "Arduino.h"
#else
#include "simulate/simulate.h"
#endif

class RFControl
{
  public:
    static void startReceiving(int interruptPin);
    static void stopReceiving();
    static bool hasData();
    static void getRaw(unsigned int **timings, unsigned int* timings_size);
    static void continueReceiving();
    static bool compressTimings(unsigned int buckets[8], unsigned int *timings, unsigned int timings_size);
    static void sendByTimings(int transmitterPin, unsigned int *timings, unsigned int timings_size, unsigned int repeats = 3);
  private:
    RFControl();
};

#endif
