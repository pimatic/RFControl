/*
  ArduinoRF.h - 
*/
#ifndef ArduinoRf_h
#define ArduinoRf_h

#include "Arduino.h"

class ArduinoRF
{
  public:
    static void startReceiving(int interruptPin);
    static bool hasData();
    static void getRaw(unsigned int **timings, unsigned int* timings_size);
    static void continueReceiving();
    static bool compressTimings(unsigned int buckets[8], unsigned int *timings, unsigned int timings_size);
  private:
    ArduinoRF();
};

#endif
