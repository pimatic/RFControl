#include <RFControl.h>

void setup() {
  Serial.begin(9600);
  RFControl::startReceiving(0);
}

void loop() {
  if(RFControl::hasData()) {
    unsigned int *timings;
    unsigned int timings_size;
    RFControl::getRaw(&timings, &timings_size);
    unsigned int buckets[8];
    RFControl::compressTimings(buckets, timings, timings_size);
    Serial.print("b: ");
    for(int i=0; i < 8; i++) {
      unsigned long bucket = buckets[i] * pulse_length_divider;
      Serial.print(bucket);
      Serial.write(' ');
    }
    Serial.print("\nt: ");
    for(int i=0; i < timings_size; i++) {
      Serial.write('0' + timings[i]);
    }
    Serial.write('\n');
    Serial.write('\n');
    RFControl::continueReceiving();
  }
}


