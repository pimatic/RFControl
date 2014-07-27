ArduinoRF
==========

Arduino library for 433mhz RF sniffing and receiving.

´´´c++
#include <ArduinoRF.h>

void setup() {
  Serial.begin(9600);
  ArduinoRF::startReceiving(0);
}

void loop() {
  if(ArduinoRF::hasData()) {
    unsigned int *timings;
    unsigned int timings_size;
    ArduinoRF::getRaw(&timings, &timings_size);
    for(int i=0; i < timings_size; i++) {
      Serial.print(timings[i]);
      Serial.write('\t');
      if((i+1)%16 == 0) {
        Serial.write('\n');
      }
    }
    Serial.write('\n');
    Serial.write('\n');
    ArduinoRF::continueReceiving();
  }
}
´´´

Outputs raw timings to serial if the same code gets received twice:

´´´
294	2630	294	242	296	1262	292	248	288	1268	286	1262	288	260	280	254	
286	1274	278	256	282	1276	280	258	276	1280	276	1272	276	270	274	1274	
274	284	276	1272	274	272	270	1278	272	272	274	262	270	1290	268	1280	
270	274	268	270	268	1288	266	270	266	1292	264	1284	264	280	266	272	
264	1302	268	272	262	1294	264	274	262	1294	262	1286	262	282	262	278	
260	1294	262	274	262	1296	262	1286	262	282	262	1286	262	282	264	274	
260	1310	260	1288	260	284	262	274	260	1298	260	276	260	1296	260	1290	
260	284	260	276	258	1300	258	278	260	1296	260	278	258	1296	260	278	
260	1292	254	10196	
´´´

It can also compress the outputs:

´´´
b: 269 2632 1286 10196 0 0 0 0 
t: 010002000202000002000200020200020002000200000202000002000202000002000200020200000200020200020000020200000200020200000200020002000203
´´´

The raw timings should be the same as outputed by pilight debug: http://wiki.pilight.org/doku.php/arctech_switch