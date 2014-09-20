#include <cstdio>
#include "../RFControl.h"

void (*interruptCallback)(void);

unsigned int sim_timings[] = {
	1, 2, 3,

	506, 2024, 506, 2024, 506, 4301, 506, 2024, 506, 4301, 506, 2277, 506, 4301, 506, 4301, 
	506, 2277, 506, 4301, 506, 2024, 506, 2024, 506, 2024, 506, 2277, 506, 4301, 506, 2024,
	506, 2024, 506, 2277, 506, 4301, 506, 2024, 506, 2024, 506, 4301, 506, 2024, 506, 4301, 
	506, 4301, 506, 2277, 506, 4301, 506, 4301, 506, 2024, 506, 4301, 506, 2024, 506, 2024, 
	506, 4301, 506, 4554, 506, 2024, 506, 2024, 506, 2277, 506, 2277, 506, 4301, 506, 2024, 
	506, 2024, 506, 4554, 506, 8602,

	506, 2024, 506, 2024, 506, 4301, 506, 2024, 506, 4301, 506, 2277, 506, 4301, 506, 4301, 
	506, 2277, 506, 4301, 506, 2024, 506, 2024, 506, 2024, 506, 2277, 506, 4301, 506, 2024,
	506, 2024, 506, 2277, 506, 4301, 506, 2024, 506, 2024, 506, 4301, 506, 2024, 506, 4301, 
	506, 4301, 506, 2277, 506, 4301, 506, 4301, 506, 2024, 506, 4301, 506, 2024, 506, 2024, 
	506, 4301, 506, 4554, 506, 2024, 506, 2024, 506, 2277, 506, 2277, 506, 4301, 506, 2024, 
	506, 2024, 506, 4554, 506, 8602,

	506, 2024, 506, 2024, 506, 4301, 506, 2024, 506, 4301, 506, 2277, 506, 4301, 506, 4301, 
	506, 2277, 506, 4301, 506, 2024, 506, 2024, 506, 2024, 506, 2277, 506, 4301, 506, 2024,
	506, 2024, 506, 2277, 506, 4301, 506, 2024, 506, 2024, 506, 4301, 506, 2024, 506, 4301, 
	506, 4301, 506, 2277, 506, 4301, 506, 4301, 506, 2024, 506, 4301, 506, 2024, 506, 2024, 
	506, 4301, 506, 4554, 506, 2024, 506, 2024, 506, 2277, 506, 2277, 506, 4301, 506, 2024, 
	506, 2024, 506, 4554, 506, 8602,

	1, 2, 3
};
size_t sim_timings_pos;
size_t sim_timings_size;

int main(int argc, const char* argv[])
{
	sim_timings_pos = 0;
	sim_timings_size = sizeof(sim_timings)/sizeof(unsigned int);

  	RFControl::startReceiving(0);

  	while(sim_timings_pos < sim_timings_size) {

  		interruptCallback();

  		if(RFControl::hasData()) {
		    unsigned int *timings;
		    unsigned int timings_size;
		    RFControl::getRaw(&timings, &timings_size);
		    printf("result: \n");
		    for(size_t i=0; i < timings_size; i++) {
		      printf("%i ", timings[i]);
		      if((i+1)%16 == 0) {
		        printf("\n");
		      }
		    }
		    printf("\n");
		    RFControl::continueReceiving();
		  }
  	}


}


void attachInterrupt(uint8_t, void (*ic)(void), int mode) {
	interruptCallback = ic;
}

unsigned long micros(void) {
	static unsigned long duration = 0;
	if(sim_timings_pos < sim_timings_size) {
		duration += sim_timings[sim_timings_pos++];
		return duration;
	} else {
		printf("No timings left...\n");
	}
	return 0;
}


void pinMode(uint8_t, uint8_t){}
void digitalWrite(uint8_t, uint8_t){}
int digitalRead(uint8_t){return 0;}
int analogRead(uint8_t){return 0;}
void analogReference(uint8_t mode){}
void analogWrite(uint8_t, int){}
void delay(unsigned long){}
void delayMicroseconds(unsigned int us){}
void detachInterrupt(uint8_t){}