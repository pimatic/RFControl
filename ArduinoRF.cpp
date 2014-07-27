#include "Arduino.h"
#include "ArduinoRF.h"

#define ARDUINORF_MAX_RECORDINGS 255
#define ARDUINORF_STATUS_WAITING 0
#define ARDUINORF_STATUS_RECORDING 1
#define ARDUINORF_STATUS_VERIFY 2
#define ARDUINORF_STATUS_DATA_READY 3

#define ARDUINORF_MIN_FOOTER_LENGTH 4000
#define ARDUINORF_MIN_PULSE_LENGTH 100 


unsigned int footer_length;
unsigned int timings[ARDUINORF_MAX_RECORDINGS];
unsigned char state;
int recording_pos;
int recording_size;  
int verify_pos;
void handleInterrupt();

void ArduinoRF::startReceiving(int interruptPin)
{
  footer_length = 0;
  state = ARDUINORF_STATUS_WAITING;
  recording_pos = 0;
  recording_size = 0;  
  verify_pos = 0;
  attachInterrupt(interruptPin, handleInterrupt, CHANGE);
}

bool ArduinoRF::hasData() 
{
  return state == ARDUINORF_STATUS_DATA_READY;
}

void ArduinoRF::getRaw(unsigned int **buffer, unsigned int* timings_size)
{
  *buffer = timings;
  *timings_size = recording_size;
}

void ArduinoRF::continueReceiving()
{
  state = ARDUINORF_STATUS_WAITING;
}

bool probablyFooter(unsigned int duration) {
  return duration >= 5000; 
}

bool matchesFooter(unsigned int duration)
{
  unsigned int footer_delta = footer_length/4;
  return (footer_length - footer_delta < duration && duration < footer_length + footer_delta);
}

void startRecording()
{
  state = ARDUINORF_STATUS_RECORDING;
  recording_pos = 0;
}

void startVerify()
{
  state = ARDUINORF_STATUS_VERIFY;
  recording_size = recording_pos;
  verify_pos = 0;
}

void handleInterrupt()
{
  static unsigned int duration;
  static unsigned long lastTime;
  long time = micros();
  duration = time - lastTime;
  lastTime = time;
  
  switch(state) {
    case ARDUINORF_STATUS_WAITING:
      if(probablyFooter(duration)) 
      {
        footer_length = duration;
        startRecording();
      }
      break;
    case ARDUINORF_STATUS_RECORDING:
      {
        if(matchesFooter(duration)) 
        {
          //If we have at least recorded 10 values:  
          if(recording_pos > 10) {
            startVerify();
          }
        } else {
          if(duration > ARDUINORF_MIN_PULSE_LENGTH) 
          {
            if(duration > footer_length) {
              footer_length = duration;
              startRecording();
            } else if(recording_pos < ARDUINORF_MAX_RECORDINGS-1) {
              timings[recording_pos] = duration;
              recording_pos++; 
            } else {
              state = ARDUINORF_STATUS_WAITING;
            }
          } else {
            state = ARDUINORF_STATUS_WAITING;
          }
        }
      }
      break;
    case ARDUINORF_STATUS_VERIFY:
      {
        unsigned int refVal = timings[verify_pos];
        unsigned int delta = refVal/2;
        if(refVal - delta < duration && duration < refVal + delta) 
        {
          verify_pos++; 
          if(verify_pos >= 10) {
            timings[recording_pos] = footer_length;
            recording_size++;
            state = ARDUINORF_STATUS_DATA_READY;
          }
        } else {
          if(probablyFooter(duration)) {
            footer_length = duration;
            startRecording();
          } else {
            state = ARDUINORF_STATUS_WAITING;
          }     
        }
      }
      break;
    }
}

bool ArduinoRF::compressTimings(unsigned int buckets[8], unsigned int *timings, unsigned int timings_size) {
  for(int j = 0; j < 8; j++ ) {
    buckets[j] = 0;
  }
  unsigned int sums[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  //sort timings into buckets, handle max 4 different pulse length
  for(int i = 0; i < timings_size; i++) 
  {
    int j = 0;
    for(; j < 8; j++) {
      unsigned int refVal = buckets[j];
      unsigned int val = timings[i];
      //if bucket is empty
      if(refVal == 0) {
        //sort into bucket
        buckets[j] = val;
        timings[i] = j;
        sums[j] += val;
        counts[j]++;
        break;
      } else {
        //check if bucket fits:
        unsigned int delta = refVal/2;
        if(refVal - delta < val && val < refVal + delta) {
          timings[i] = j;
          sums[j] += val;
          counts[j]++;
          break;
        }
      }
      //try next..
    }
    if(j == 8) {
      //we have not found a bucket for this timing, exit...
      return false;
    }
  }
  for(int j = 0; j < 8; j++) {
    if(counts[j] != 0) {
      buckets[j] = sums[j] / counts[j];
    }
  }
  return true;
}