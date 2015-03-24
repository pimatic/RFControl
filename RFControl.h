/*
  RFControl.h - 
*/
#ifndef ArduinoRf_h
#define ArduinoRf_h
#ifdef RF_CONTROL_SIMULATE_ARDUINO
#include "simulate/simulate.h"
#elif RF_CONTROL_VARDUINO
#include "simulate/simulate.h"
#else
#include "Arduino.h"
#endif

class RFControl
{
  public:
    static unsigned int getPulseLengthDivider();
    static void startReceiving(int interruptPin);
    static void stopReceiving();
    static bool hasData();
    static void getRaw(unsigned int **timings, unsigned int* timings_size);
    static void continueReceiving();
    static bool compressTimings(unsigned int buckets[8], unsigned int *timings, unsigned int timings_size);
    static bool compressTimingsAndSortBuckets(unsigned int buckets[8], unsigned int *timings, unsigned int timings_size);
    static void sendByTimings(int transmitterPin, unsigned int *timings, unsigned int timings_size, unsigned int repeats = 3);
    static void sendByCompressedTimings(int transmitterPin, unsigned int* buckets, char* compressTimings, unsigned int repeats = 3); 
    static unsigned int getLastDuration();
	static bool existNewDuration();
  private:
    RFControl();
};

#endif
