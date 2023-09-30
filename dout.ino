#include "dout.h"
#include <arduino.h>
void dout_setup(void) 
{
  int i;
  nValvePins[0] = 4;
  nValvePins[1] = 16;
  nValvePins[2] = 17;
  nValvePins[3] = 5;
  nValvePins[4] = 18;
  nValvePins[5] = 19;
  nValvePins[6] = 21;

  pinMode(PUMP_PIN, OUTPUT);    
  
  for(i=0;i<7;i++)
     pinMode(nValvePins[i], OUTPUT);

  delay(500);
  digitalWrite(PUMP_PIN, HIGH); 
 
  for(i=0;i<7;i++)
    digitalWrite(nValvePins[i], HIGH);

}

void pump_on(void)
{
    digitalWrite(PUMP_PIN, 0x0); 
}

void pump_off(void)
{
    digitalWrite(PUMP_PIN, 0x1); 
}

void Valve_Open(int nValve)
{
    digitalWrite(nValvePins[nValve], OPEN); 
}
void Valve_Close(int nValve)
{
      digitalWrite(nValvePins[nValve], CLOSED); 
}
