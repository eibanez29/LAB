# LAB
ESP32 - Codigo para maquina de lavado de tanques de cerveza Kooch

Se define una secuencia de lavado a traves de una estructura de datos

'''

//Estructura para el manejo de tipos de lavado
struct stEtapaDeLavado
{
  int uiSegundos; //Tiempo de ejecucion de la etapa en segundos
  String strLiteralDescription; //Descripcion la etapa
  bool bValves_Status[MAX_NUM_VALVES];
  unsigned int uiEstado_Bomba;
};

'''

Los tiempos de lavado se guardan en memoria no volatil.

# Asignacioon de pines
La asignacion de pines se puede ver en el archivo dout.h y dout.ino

  nValvePins[0] = 4;

  nValvePins[1] = 16;

  nValvePins[2] = 17;

  nValvePins[3] = 5;

  nValvePins[4] = 18;

  nValvePins[5] = 19;

  nValvePins[6] = 21;

# Comandos por el puerto serie

start - Inicializa lavado completo

inilav:X - inicializa lavado X donde X es el numero de lavado 

config:XYY - configura el tiemo de lavado X lavado YY tiempo ensegundos

pumpon - inicializa la bomba

pumpoff - apaga la bomba

valvopen:X - abre la válvula

valvclose:X - cierra la válvula

stop - se ceirran las valvulas y se apaga ala bomba

EEPROM - muestra el contenido de la EEPROM

Grabar - Graba los tiempos de los lavados a la memoria EEPROM

Lavlist - Muestra la lista de lavados

# Commandos por telegram

/start

/manualoptions

/lavar

/status
