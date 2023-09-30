#include <EEPROM.h>
#include "LAB_EEPROM.h"

void LAB_EEPROM_Init(void)
{
  EEPROM.begin(EEPROM_SIZE);
}

void LAB_EEPROM_Save_LavTime(int nLavNro, int nTime)
{ 
  unsigned int uiAdress;

  uiAdress = nLavNro*2;

  EEPROM.write(uiAdress, (nTime & 0xFF));
  EEPROM.write(uiAdress+1, (nTime>>8) & 0xFF);

  EEPROM.commit();

}

int LAB_EEPROM_Read_LavTime(int nLavNro)
{
  int nLavTime;
  unsigned int uiAdress;
  unsigned char bLow;
  unsigned char bHigh;
  

   uiAdress = nLavNro*2;

   bLow=EEPROM.read(uiAdress);
   bHigh=EEPROM.read(uiAdress+1);

   nLavTime= (int) ((bHigh<<8) | bLow);
   return nLavTime;
}
