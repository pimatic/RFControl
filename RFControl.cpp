#include "RFControl.h"

#define MAX_RECORDINGS 512
#define STATUS_WAITING 0
#define STATUS_RECORDING_0 1
#define STATUS_RECORDING_1 2
#define STATUS_RECORDING_2 3
#define STATUS_RECORDING_3 4
#define STATUS_RECORDING_END 5

#define MIN_FOOTER_LENGTH 3500
#define MIN_PULSE_LENGTH 100 


unsigned int footer_length;
unsigned int timings[MAX_RECORDINGS];
unsigned char state;
int interruptPin = -1;
int data_start[5];
int data_end[5];
bool Pack0EqualPack1 = false;
bool Pack0EqualPack2 = false;
bool Pack0EqualPack3 = false;
bool Pack1EqualPack2 = false;
bool Pack1EqualPack3 = false;
bool data1_ready = false;
bool data2_ready = false;
void handleInterrupt();

void RFControl::startReceiving(int _interruptPin) {
  footer_length = 0;
  state = STATUS_WAITING;
  data_end[0] = 0;
  data_end[1] = 0;
  data_end[2] = 0;
  data_end[3] = 0;
  data_start[0] = 0;
  data_start[1] = 0;
  data_start[2] = 0;
  data_start[3] = 0; 
  data1_ready = false;
  data2_ready = false;
  if(interruptPin != -1) {
    detachInterrupt(interruptPin);   
  }
  interruptPin = _interruptPin;
  attachInterrupt(interruptPin, handleInterrupt, CHANGE);
}

void RFControl::stopReceiving() {
  if(interruptPin != -1) {
    detachInterrupt(interruptPin);   
  }
  interruptPin = -1;
}

bool RFControl::hasData() {
  return (data1_ready || data2_ready);
}

void RFControl::getRaw(unsigned int **buffer, unsigned int* timings_size) {
  *buffer = timings;
  *timings_size = data_end[0];
  data1_ready = false;
  data2_ready = false;
}

void RFControl::continueReceiving() {
  if(state == STATUS_RECORDING_END)
  {
    state = STATUS_WAITING;
    data1_ready = false;
    data2_ready = false;
  }
}

bool probablyFooter(unsigned int duration) {
  return duration >= MIN_FOOTER_LENGTH; 
}

bool matchesFooter(unsigned int duration) {
  unsigned int footer_delta = footer_length/4;
  return (footer_length - footer_delta < duration && duration < footer_length + footer_delta);
}

void startRecording(unsigned int duration) {
  #ifdef RF_CONTROL_SIMULATE_ARDUINO
  printf(" => start recoding");
  #endif
  footer_length = duration;
  data_end[0] = 0;
  data_end[1] = 0;
  data_end[2] = 0;
  data_end[3] = 0;
  data_start[0] = 0;
  data_start[1] = 0;
  data_start[2] = 0;
  data_start[3] = 0;
  Pack0EqualPack3 = true;
  Pack1EqualPack3 = true;
  Pack0EqualPack2 = true;
  Pack1EqualPack2 = true;
  Pack0EqualPack1 = true;
  data1_ready = false;
  data2_ready = false;
  state = STATUS_RECORDING_0;
}

void recording(int duration, int package) {
  if (matchesFooter(duration)) //Das timing wird mit +-25% des footers geprüft.
  {
    //Paket ist komplett!!!!
    //Wenn das Timing gleich des footers ist machen wir hier weiter
    timings[data_end[package]] = duration;
    data_start[package + 1] = data_end[package] + 1;
    data_end[package + 1] = data_start[package + 1];

    //Haben wir mehr als 32 timings und der anfang und das ende sind ein footer haben wir wohl ein komplettes Packet erhalten. 
    //Weniger als 32 -> der gespeicherte footer war wohl keiner.  Alle Werte werden zurückgesetzt.
    if (data_end[package] - data_start[package] >= 32)
    {
      if (state == STATUS_RECORDING_3)
        state = STATUS_RECORDING_END;
      else
      {
        state = STATUS_RECORDING_0 + package + 1;
      }
    }
    else
    {
	data_end[package] = data_start[package];
      switch (package)
      {
        case 0:
          startRecording(duration); //restart
          break;
        case 1:
          Pack0EqualPack1 = true;
          break;
        case 2:
          Pack0EqualPack2 = true;
          Pack1EqualPack2 = true;
          break;
        case 3:
          Pack0EqualPack3 = true;
          Pack1EqualPack3 = true;
          break;
      }
    }
  }
  else
  {
    //Sollte das timing dem footer nicht entsprechen wird hier weiter gemacht.
    //Ist das empfangene timing größer als der gespeicherte footer, dann ist er kein footer und wir müssen restarten.
    if (duration > footer_length)
    {
      startRecording(duration);
    }
    //Normales verfahren.
    else if (data_end[package] < MAX_RECORDINGS - 1)
    {
      timings[data_end[package]] = duration;
      data_end[package]++;
    }
    //Der speicher ist vollgelaufen, wir haben aber kein gültiges paket empfangen. STOP
    else
    {
      state = STATUS_WAITING;
    }
  }
}

void verification1() {
  int pos = data_end[1] - 1 - data_start[1];
  if (Pack0EqualPack1 && pos >= 0)
  {
    int refVal = timings[pos];
    int mainVal = timings[data_end[1] - 1];
    int delta = refVal / 8 + refVal / 4; //+-37,5%
    if (refVal - delta > mainVal || mainVal > refVal + delta)
    {
       //werte passen nicht
       Pack0EqualPack1 = false;
    }
    if (state == STATUS_RECORDING_2 && Pack0EqualPack1 == true)
    {
      data1_ready = true;
      state = STATUS_RECORDING_END;
    }
  }
}

void verification2() {
  int refVal = timings[data_end[2] - 1];
  int delta = refVal / 8 + refVal / 4; //+-37,5%
  int refVal_min = refVal - delta;
  int refVal_max = refVal + delta;
  int pos = data_end[2] - 1 - data_start[2];

  if (Pack0EqualPack2 && pos >= 0)
  {
    int mainVal = timings[pos];
    if (refVal_min > mainVal || mainVal > refVal_max)
    {
      //werte passen nicht
      Pack0EqualPack2 = false;
    }
    if (state == STATUS_RECORDING_3 && Pack0EqualPack2 == true)
    {
      data1_ready = true;
    }
  }
  pos = pos + data_start[1];
  if (Pack1EqualPack2 && pos >= 0)
  {
    int mainVal = timings[pos];
    if (refVal_min > mainVal || mainVal > refVal_max)
    {
      //werte passen nicht
      Pack1EqualPack2 = false;
    }
    if (state == STATUS_RECORDING_3 && Pack1EqualPack2 == true)
    {
      data2_ready = true;
    }
  }
}

void verification3() {
  int refVal = timings[data_end[3] - 1]; //Wert vom 4ten Pack
  int delta = refVal / 8 + refVal / 4; //+-37,5%
  int refVal_min = refVal - delta;
  int refVal_max = refVal + delta;
  int pos = data_end[3] - 1 - data_start[3];

  if (!Pack0EqualPack2 && Pack0EqualPack3 && pos >= 0)
  {
    int mainVal = timings[pos];
    if (refVal_min > mainVal || mainVal > refVal_max)
    {
       //werte passen nicht
       Pack0EqualPack3 = false;
    }
    if (state == STATUS_RECORDING_END && Pack0EqualPack3 == true)
    {
      data1_ready = true;
    }
  }
  pos = pos + data_start[1];
  if (!Pack1EqualPack2 && Pack1EqualPack3 && pos >= 0)
  {
    int mainVal = timings[pos];
    if (refVal_min > mainVal || mainVal > refVal_max)
    {
      //werte passen nicht
      Pack1EqualPack3 = false;
    }
    if (state == STATUS_RECORDING_END && Pack1EqualPack3 == true)
    {
      data2_ready = true;
    }
  }
}

void handleInterrupt() {
  static unsigned long lastTime;
  long currentTime = micros();
  unsigned int duration = currentTime - lastTime;
  
  #ifdef RF_CONTROL_SIMULATE_ARDUINO
  printf("%s: recording=%i verify=%i duration=%i", sate2string[state], recording_pos, verify_pos, duration);
  #endif
  if (duration >= MIN_PULSE_LENGTH)
  {
    lastTime = currentTime;
    switch (state)
    {
    case STATUS_WAITING:
      if (probablyFooter(duration))
        startRecording(duration);
      break;
    case STATUS_RECORDING_0:
      recording(duration, 0);
      break;
    case STATUS_RECORDING_1:
      recording(duration, 1);
      verification1();
      break;
    case STATUS_RECORDING_2:
      recording(duration, 2);
      verification2();
      break;
    case STATUS_RECORDING_3:
      recording(duration, 3);
      verification3();
      break;
    }
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
