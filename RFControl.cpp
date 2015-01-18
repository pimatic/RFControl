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
int interruptPin = -1;
void handleInterrupt();

void RFControl::startReceiving(int _interruptPin)
{
  footer_length = 0;
  state = STATUS_WAITING;
  recording_pos = 0;
  recording_size = 0;  
  verify_pos = 0;
  if(interruptPin != -1) {
    detachInterrupt(interruptPin);   
  }
  interruptPin = _interruptPin;
  attachInterrupt(interruptPin, handleInterrupt, CHANGE);
}

void RFControl::stopReceiving()
{
  if(interruptPin != -1) {
    detachInterrupt(interruptPin);   
  }
  interruptPin = -1;
  state = STATUS_WAITING;
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
  #ifdef RF_CONTROL_SIMULATE_ARDUINO
  printf(" => start recoding");
  #endif
  footer_length = duration;
  recording_pos = 0;
  state = STATUS_RECORDING;
}

void startVerify()
{
  #ifdef RF_CONTROL_SIMULATE_ARDUINO
  printf(" => start verify (recording_size=%i)", recording_pos);
  #endif
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

  #ifdef RF_CONTROL_SIMULATE_ARDUINO
  printf("%s: recording=%i verify=%i duration=%i", sate2string[state], recording_pos, verify_pos, duration);
  #endif
  
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
          timings[recording_pos] = duration;
          recording_pos++; 

          //If we have at least recorded 32 values:  
          if(recording_pos >= 32) {
            startVerify();
          } else {
            // so seems it was not a footer, just a pulse of the regular recording, 
            startRecording(duration);
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
            state = STATUS_DATA_READY;
          } else {
            // keep recording parallel for the case verification fails.
            if(recording_pos < MAX_RECORDINGS-1) {
              timings[recording_pos] = duration;
              recording_pos++;
            }
          }
        } else {
          if(probablyFooter(duration)) {
            // verification failed but it could be a footer, try to verify again with the
            // parallel recorded results
            if(recording_pos < MAX_RECORDINGS-1) {
              timings[recording_pos] = duration;
              recording_pos++; 
              startVerify();
            } else {
              state = STATUS_WAITING;
            }
          } else {
            state = STATUS_WAITING;
          }     
        }
      }
      break;
    }
    
    #ifdef RF_CONTROL_SIMULATE_ARDUINO
    printf("\n");
    #endif
}

bool RFControl::compressTimings(unsigned int buckets[8], unsigned int *timings, unsigned int timings_size) {
  for(int j = 0; j < 8; j++ ) {
    buckets[j] = 0;
  }
  unsigned long sums[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  //sort timings into buckets, handle max 8 different pulse length
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

bool RFControl::compressTimingsAndSortBuckets(unsigned int buckets[8], unsigned int *timings, unsigned int timings_size) {
  //clear buckets
  for(int j = 0; j < 8; j++ ) {
    buckets[j] = 0;
  }
  //define arrays too calc the average value from the buckets
  unsigned long sums[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  //sort timings into buckets, handle max 8 different pulse length
  for(unsigned int i = 0; i < timings_size; i++) 
  {
    int j = 0;
    //timings need only there to load
    unsigned int val = timings[i];
    for(; j < 8; j++) {
      unsigned int refVal = buckets[j];
      //if bucket is empty
      if(refVal == 0) {
        //sort into bucket
        buckets[j] = val;
        sums[j] += val;
        counts[j]++;
        break;
      } else {
        //check if bucket fits:
        //its allowed round about 37,5% diff
        unsigned int delta = refVal/4 + refVal/8;
        if(refVal - delta < val && val < refVal + delta) {
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
  //calc the average value from the buckets
  for(int j = 0; j < 8; j++) {
    if(counts[j] != 0) {
      buckets[j] = sums[j] / counts[j];
    }
  }
  //buckets are defined
  //lets scramble a little bit
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 7; j++) {
      if(buckets[j] > buckets[j+1]){
        unsigned int temp = buckets[j];
        buckets[j] = buckets[j+1];
        buckets[j+1] = temp;
      }
    }
  }
  // now the buckets are ordered by size from low to high.
  // but the zero ist first. lets move this back
  // find first value
  int first = 0;
  for(int i = 0; i < 8; i++){
    if(buckets[i] != 0){
    first = i;
    break;
    }
  }
  //copy buckets to the start of the array
  int end = 8 - first;
  for(int i = 0; i < end; i++){
    buckets[i] = buckets[first];
    buckets[first] = 0;
    first++;
  }
  
  //and now we can assign the timings with the position_value from the buckets
  //pre calc ref values. save time
  unsigned int ref_Val_h[8];
  unsigned int ref_Val_l[8];
  for(int i = 0; i<8;i++) {
    unsigned int refVal = buckets[i];
    //check if bucket fits:
    unsigned int delta = refVal/4 + refVal/8;
    ref_Val_h[i] = refVal + delta;
    ref_Val_l[i] = refVal - delta;
  }
  for(unsigned int i = 0; i < timings_size; i++) 
  {
    unsigned int val = timings[i];
    for(int j = 0; j < 8; j++) {
      if(ref_Val_l[j] < val && val < ref_Val_h[j]) {
        timings[i] = j;
        break;
      }
    }
  }
  return true;
}

void RFControl::sendByTimings(int transmitterPin, unsigned int *timings, unsigned int timings_size, unsigned int repeats) {
  // listen before talk

  int _interruptPin = interruptPin;
  if(state != STATUS_WAITING) {
    //wait till no rf message is in the air
    delayMicroseconds(100);
  }

  // stop receiving while sending
  if(_interruptPin != -1) {
    stopReceiving();
  }

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

  // enable reciving again
  if(_interruptPin != -1) {
    startReceiving(_interruptPin);
  }
}

