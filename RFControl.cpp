#include "RFControl.h"

#if (defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)) && !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 400   //In combination with Homeduino maximum 490*2Byte are possible. Higher values block the arduino
#endif
#if (defined(__AVR_ATmega32U4__) || defined(TEENSY20) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)) && !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 512   //on the bigger arduino we have enough SRAM
#endif
#if !defined(MAX_RECORDINGS)
  #define MAX_RECORDINGS 255   // fallback for undefined Processor.
#endif

#define STATUS_WAITING 0
#define STATUS_RECORDING_0 1
#define STATUS_RECORDING_1 2
#define STATUS_RECORDING_2 3
#define STATUS_RECORDING_3 4
#define STATUS_RECORDING_END 5

#define PULSE_LENGTH_DIVIDER 4

#define MIN_FOOTER_LENGTH (3500 / PULSE_LENGTH_DIVIDER)
#define MIN_PULSE_LENGTH (100 / PULSE_LENGTH_DIVIDER)

unsigned int footer_length;
unsigned int timings[MAX_RECORDINGS];
unsigned long lastTime = 0;
unsigned char state;
unsigned int duration = 0;
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
bool skip = false;
bool new_duration = false;
void handleInterrupt();

unsigned int RFControl::getPulseLengthDivider() {
  return PULSE_LENGTH_DIVIDER;
}

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
  state = STATUS_WAITING;
}

bool RFControl::hasData() {
  return (data1_ready || data2_ready);
}

void RFControl::getRaw(unsigned int **buffer, unsigned int* timings_size) {
  if (data1_ready){
    *buffer = &timings[0];
    *timings_size = data_end[0] + 1;
    data1_ready = false;
  }
  else if (data2_ready)
  {
    *buffer = &timings[data_start[1]];
    *timings_size = data_end[1] - data_start[1] + 1;
    data2_ready = false;
  }
}

void RFControl::continueReceiving() {
  if(state == STATUS_RECORDING_END)
  {
    state = STATUS_WAITING;
    data1_ready = false;
    data2_ready = false;
  }
}

unsigned int RFControl::getLastDuration(){
	new_duration = false;
	return duration;
}

bool RFControl::existNewDuration(){
	return new_duration;
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

void recording(unsigned int duration, int package) {
#ifdef RF_CONTROL_SIMULATE_ARDUINO
  //nice string builder xD
  printf("%s:", sate2string[state]);
  if (data_end[package] < 10)
    printf(" rec_pos=  %i", data_end[package]);
  else if (data_end[package] < 100)
    printf(" rec_pos= %i", data_end[package]);
  else if (data_end[package] < 1000)
    printf(" rec_pos=%i", data_end[package]);
  int pos = data_end[package] - data_start[package];
  if (pos < 10)
    printf(" pack_pos=  %i", pos);
  else if (pos < 100)
    printf(" pack_pos= %i", pos);
  else if (pos < 1000)
    printf(" pack_pos=%i", pos);

  if (duration < 10)
    printf(" duration=    %i", duration);
  else if (duration < 100)
    printf(" duration=   %i", duration);
  else if (duration < 1000)
    printf(" duration=  %i", duration);
  else if (duration < 10000)
    printf(" duration= %i", duration);
  else if (duration < 100000)
    printf(" duration=%i", duration);
#endif
  if (matchesFooter(duration)) //test for footer (+-25%).
  {
    //Package is complete!!!!
    timings[data_end[package]] = duration;
    data_start[package + 1] = data_end[package] + 1;
    data_end[package + 1] = data_start[package + 1];

    //Received more than 32 timings and start and end are the same footer then enter next state
    //less than 32 timings -> restart the package.
    if (data_end[package] - data_start[package] >= 32)
    {
      if (state == STATUS_RECORDING_3) {
        state = STATUS_RECORDING_END;
      }else
      {
        state = STATUS_RECORDING_0 + package + 1;
      }
    }
    else
    {
      #ifdef RF_CONTROL_SIMULATE_ARDUINO
        printf(" => restart package");
      #endif
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
    //duration isnt a footer? this is the way.
    //if duration higher than the saved footer then the footer isnt a footer -> restart.
    if (duration > footer_length)
    {
      startRecording(duration);
    }
    //normal
    else if (data_end[package] < MAX_RECORDINGS - 1)
    {
      timings[data_end[package]] = duration;
      data_end[package]++;
    }
    //buffer reached end. Stop recording.
    else
    {
      state = STATUS_WAITING;
    }
  }
}



void verify(bool *verifiystate, bool *datastate, unsigned int refVal_max, unsigned int refVal_min, int pos, int package){
  if (*verifiystate && pos >= 0)
  {
    unsigned int mainVal = timings[pos];
    if (refVal_min > mainVal || mainVal > refVal_max)
    {
      //werte passen nicht
      *verifiystate = false;
    }
    #ifdef RF_CONTROL_SIMULATE_ARDUINO
    printf(" - verify = %s", *verifiystate ? "true" : "false");
    #endif
    if (state == (STATUS_RECORDING_0 + package + 1) && *verifiystate == true)
    {
      #ifdef RF_CONTROL_SIMULATE_ARDUINO
      printf("\nPackage are equal.");
      #endif
      *datastate = true;
    }
  }
}

void verification(int package) {
  int refVal = timings[data_end[package] - 1];
  int delta = refVal / 8 + refVal / 4; //+-37,5%
  int refVal_min = refVal - delta;
  int refVal_max = refVal + delta;
  int pos = data_end[package] - 1 - data_start[package];

  switch (package)
  {
  case 1:
    verify(&Pack0EqualPack1, &data1_ready, refVal_max, refVal_min, pos, package);
    break;
  case 2:
    verify(&Pack0EqualPack2, &data1_ready, refVal_max, refVal_min, pos, package);
    verify(&Pack1EqualPack2, &data2_ready, refVal_max, refVal_min, pos, package);
    if (state == STATUS_RECORDING_3 && data1_ready == false && data2_ready == false) {
      state = STATUS_WAITING;
    }
    break;
  case 3:
    if (!Pack0EqualPack2)
      verify(&Pack0EqualPack3, &data1_ready, refVal_max, refVal_min, pos, package);
    if (!Pack1EqualPack2)
      verify(&Pack1EqualPack3, &data2_ready, refVal_max, refVal_min, pos, package);
    if (state == STATUS_RECORDING_END && data1_ready == false && data2_ready == false) {
      state = STATUS_WAITING;
    }
    break;
  }
}

void handleInterrupt() {
  //digitalWrite(9, HIGH);
  unsigned long currentTime = micros();
  duration = (currentTime - lastTime) / PULSE_LENGTH_DIVIDER;
  //lastTime = currentTime;
  if (skip) {
    skip = false;
    return;
  }
  if (duration >= MIN_PULSE_LENGTH)
  {
	new_duration = true;
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
      verification(1);
      break;
    case STATUS_RECORDING_2:
      recording(duration, 2);
      verification(2);
      break;
    case STATUS_RECORDING_3:
      recording(duration, 3);
      verification(3);
      break;
    }
  }
  else
    skip = true;
  //digitalWrite(9, LOW);
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

void listenBeforeTalk()
{
  // listen before talk
  unsigned long waited = 0;
  if(interruptPin != -1) {
    while(state > STATUS_RECORDING_0 && state != STATUS_RECORDING_END) {
      //wait till no rf message is in the air
      waited += 5;
      delayMicroseconds(3); // 5 - some micros for other stuff
      // don't wait longer than 5sec
      if(waited > 5000000) {
        break;
      }
      // some delay between the message in air and the new message send
      // there could be additional repeats following so wait some more time
      if(state <= STATUS_RECORDING_0 || state == STATUS_RECORDING_END) {
        waited += 1000000;
        delay(1000);
      }
    }
    // stop receiving while sending, this method preserves the recording state
    detachInterrupt(interruptPin);   
  }
  // this prevents loosing the data in the receiving buffer, after sending
  if(data1_ready || data2_ready) {
    state = STATUS_RECORDING_END;
  }
  // Serial.print(waited);
  // Serial.print(" ");
}

void afterTalk()
{
  // enable reciving again
  if(interruptPin != -1) {
    attachInterrupt(interruptPin, handleInterrupt, CHANGE);
  }
}

void delayMicrosecondsLong(unsigned int time_to_wait){
  //  delayMicroseconds() only works up to 16383 micros
  // https://github.com/pimatic/rfcontroljs/issues/29#issuecomment-85460916
  while(time_to_wait > 16000) {
    delayMicroseconds(16000);
    time_to_wait -= 16000;
  }
  delayMicroseconds(time_to_wait);
}


void RFControl::sendByCompressedTimings(int transmitterPin,unsigned int* buckets, char* compressTimings, unsigned int repeats) {
  listenBeforeTalk();
  unsigned int timings_size = strlen(compressTimings);
  pinMode(transmitterPin, OUTPUT);
  for(unsigned int i = 0; i < repeats; i++) {
    digitalWrite(transmitterPin, LOW);
    int state = LOW;
    for(unsigned int j = 0; j < timings_size; j++) {
      state = !state;
      digitalWrite(transmitterPin, state);
      unsigned int index = compressTimings[j] - '0';
      delayMicrosecondsLong(buckets[index]);
    }
  }
  digitalWrite(transmitterPin, LOW);
  afterTalk();
}


void RFControl::sendByTimings(int transmitterPin, unsigned int *timings, unsigned int timings_size, unsigned int repeats) {
  listenBeforeTalk();

  pinMode(transmitterPin, OUTPUT);
  for(unsigned int i = 0; i < repeats; i++) {
    digitalWrite(transmitterPin, LOW);
    int state = LOW;
    for(unsigned int j = 0; j < timings_size; j++) {
      state = !state;
      digitalWrite(transmitterPin, state);
      delayMicrosecondsLong(timings[j]);
    }
  }
  digitalWrite(transmitterPin, LOW);
  afterTalk();
}

