#ifndef LAB_EEPROM
  #define LAB_EEPROM
#define EEPROM_SIZE 18 //numero de lavados x 2 ( 2 bytes por lavado para guardar los tiempos)

void LAB_EEPROM_Init(void);
void LAB_EEPROM_Save_LavTime(int nLavNro, int nTime);
int LAB_EEPROM_Read_LavTime(int nLavNro);



#endif  //#ifndef LAB_EEPROM

