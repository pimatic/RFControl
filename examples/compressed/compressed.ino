#include <ArduinoRF.h>

void setup() {
  Serial.begin(6900);
  ArduinoRF::startReceiving(0);
}

void loop() {
  if(ArduinoRF::hasData()) {
    unsigned int *timings;
    unsigned int timings_size;
    ArduinoRF::getRaw(&timings, &timings_size);
    unsigned int buckets[8];
    ArduinoRF::compressTimings(buckets, timings, timings_size);
    Serial.print("b: ");
    for(int i=0; i < 8; i++) {
      Serial.print(buckets[i]);
      Serial.write(' ');
    }
    Serial.print("\nt: ");
    for(int i=0; i < timings_size; i++) {
      Serial.write('0' + timings[i]);
    }
    Serial.write('\n');
    Serial.write('\n');
    ArduinoRF::continueReceiving();
  }
}


