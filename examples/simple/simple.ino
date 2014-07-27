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
    for(int i=0; i < timings_size; i++) {
      Serial.ṕrint(timings[i]);
      Serial.write(' ');
      if((i+1)%16 == 0) {
        Serial.write('\n');
      }
    }
    Serial.write('\n');
    Serial.write('\n');
    ArduinoRF::continueReceiving();
  }
}


