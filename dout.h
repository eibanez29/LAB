#ifndef DOUT_H
   #define DOUT_H

#define PUMP_PIN 2

#define OPEN 0x0
#define CLOSED 0x1

int nValvePins[7];
void dout_setup(void);
void pump_on(void);
void pump_off(void);
void Valve_Open(int nValve);
void Valve_Close(int nValve);

#endif  //DOUT_H
