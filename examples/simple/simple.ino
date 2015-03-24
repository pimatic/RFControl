#include <RFControl.h>

void setup() {
  Serial.begin(9600);
  RFControl::startReceiving(0);
}

void loop() {
  if(RFControl::hasData()) {
    unsigned int *timings;
    unsigned int pulse_length_divider = RFControl::getPulseLengthDivider();
    RFControl::getRaw(&timings, &timings_size);
    for(int i=0; i < timings_size; i++) {
      unsigned long timing = timings[i] * pulse_length_divider;
      Serial.print(timing);
      Serial.write(' ');
      if((i+1)%16 == 0) {
        Serial.write('\n');
      }
    }
    Serial.write('\n');
    Serial.write('\n');
    RFControl::continueReceiving();
  }
}


