#include "Arduino.h"
#include "RFControl.h"

#define MAX_RECORDINGS 255
#define STATUS_WAITING 0
#define STATUS_RECORDING 1
#define STATUS_VERIFY 2
#define STATUS_DATA_READY 3

#define MIN_FOOTER_LENGTH 4000
#define MIN_PULSE_LENGTH 100 


unsigned int footer_length;
unsigned int timings[MAX_RECORDINGS];
unsigned char state;
int recording_pos;
int recording_size;  
int verify_pos;
void handleInterrupt();

void RFControl::startReceiving(int interruptPin)
{
  footer_length = 0;
  state = STATUS_WAITING;
  recording_pos = 0;
  recording_size = 0;  
  verify_pos = 0;
  attachInterrupt(interruptPin, handleInterrupt, CHANGE);
}

bool RFControl::hasData() 
{
  return state == STATUS_DATA_READY;
}

void RFControl::getRaw(unsigned int **buffer, unsigned int* timings_size)
{
  *buffer = timings;
  *timings_size = recording_size;
}

void RFControl::continueReceiving()
{
  state = STATUS_WAITING;
}

bool probablyFooter(unsigned int duration) {
  return duration >= MIN_FOOTER_LENGTH; 
}

bool matchesFooter(unsigned int duration)
{
  unsigned int footer_delta = footer_length/4;
  return (footer_length - footer_delta < duration && duration < footer_length + footer_delta);
}

void startRecording(unsigned int duration)
{
  footer_length = duration;
  recording_pos = 0;
  state = STATUS_RECORDING;
}

void startVerify()
{
  state = STATUS_VERIFY;
  recording_size = recording_pos;
  verify_pos = 0;
}

void handleInterrupt()
{
  static unsigned long lastTime;
  long currentTime = micros();
  unsigned int duration = currentTime - lastTime;
  lastTime = currentTime;
  
  switch(state) {
    case STATUS_WAITING:
      if(probablyFooter(duration)) 
      {
        startRecording(duration);
      }
      break;
    case STATUS_RECORDING:
      {
        if(matchesFooter(duration)) 
        {
          //If we have at least recorded 10 values:  
          if(recording_pos >= 8) {
            startVerify();
          }
        } else {
          if(duration > MIN_PULSE_LENGTH) 
          {
            if(duration > footer_length) {
              startRecording(duration);
            } else if(recording_pos < MAX_RECORDINGS-1) {
              timings[recording_pos] = duration;
              recording_pos++; 
            } else {
              state = STATUS_WAITING;
            }
          } else {
            state = STATUS_WAITING;
          }
        }
      }
      break;
    case STATUS_VERIFY:
      {
        unsigned int refVal = timings[verify_pos];
        unsigned int delta = refVal/4 + refVal/8;
        if(refVal - delta < duration && duration < refVal + delta) 
        {
          verify_pos++; 
          if(verify_pos == recording_size) {
            timings[recording_pos] = footer_length;
            recording_size++;
            state = STATUS_DATA_READY;
          }
        } else {
          if(probablyFooter(duration)) {
            startRecording(duration);
          } else {
            state = STATUS_WAITING;
          }     
        }
      }
      break;
    }
}

bool RFControl::compressTimings(unsigned int buckets[8], unsigned int *timings, unsigned int timings_size) {
  for(int j = 0; j < 8; j++ ) {
    buckets[j] = 0;
  }
  unsigned int sums[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  //sort timings into buckets, handle max 4 different pulse length
  for(unsigned int i = 0; i < timings_size; i++) 
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
        unsigned int delta = refVal/4 + refVal/8;
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

void RFControl::sendByTimings(int transmitterPin, unsigned int *timings, unsigned int timings_size, unsigned int repeats) {
  pinMode(transmitterPin, OUTPUT);
  for(unsigned int i = 0; i < repeats; i++) {
    digitalWrite(transmitterPin, LOW);
    int state = LOW;
    for(unsigned int j = 0; j < timings_size; j++) {
      state = !state;
      digitalWrite(transmitterPin, state);
      delayMicroseconds(timings[j]);
    }
  }
  digitalWrite(transmitterPin, LOW);
}