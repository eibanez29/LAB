#include <Arduino.h>
#include <string.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "dout.h"
#include "LAB_EEPROM.h"

//Toquen del bot @kooch_lav_bot
#define BOT_TOKEN "5133063676:AAG5_hEMbJSor9mTom6HTcuyVRBkj1thiWY"
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

TaskHandle_t Task1; //Control - maquina de estados para los lavados
TaskHandle_t Task2; //Control de comandos por pueta serie
TaskHandle_t Task3; //Control de conexión por WiFi

volatile float fVersion = 0.2;
volatile bool serial_on=true;

#define MAX_NUM_VALVES 7
#define BOMBA_ON 0x100A
#define BOMBA_OFF 0x02FE
//Estructura para el manejo de tipos de lavado
struct stEtapaDeLavado
{
  int uiSegundos; //Tiempo de ejecucion de la etapa en segundos
  String strLiteralDescription; //Descripcion la etapa
  bool bValves_Status[MAX_NUM_VALVES];
  unsigned int uiEstado_Bomba;
};
#define MAX_NUM_LAVADOS 9
//Lista de etapas de lavado
stEtapaDeLavado sListadeLavados[MAX_NUM_LAVADOS];

#define LAV_INIT 0x01
#define LAV_TIME_CTRL 0x02  
#define LAV_STAND_BY 0x03

volatile int nLavStatus=LAV_STAND_BY;

#define MAX_STEPS 10

volatile int nCurrent_step;
volatile int nMaxExecution_steps;
volatile int nListaPasos[MAX_STEPS];
#define MAX_LAV_TIME 300
#define DEFAULT_LAV_TIME 30
/*
Configura la lista de lavados segun las valvulas abiertas, bomba on/Off, tiempo de lavado con valores por defecto
Lee de la memoria EEPROM los tiempos de lavados
*/
void ConfigListadeLavados(void)
{
  int nAuxTime;
  sListadeLavados[0].strLiteralDescription="Descarga Residual de cerveza";
  sListadeLavados[0].uiEstado_Bomba = BOMBA_OFF;
  sListadeLavados[0].bValves_Status[0]=false;
  sListadeLavados[0].bValves_Status[1]=true;
  sListadeLavados[0].bValves_Status[2]=true;
  sListadeLavados[0].bValves_Status[3]=false;
  sListadeLavados[0].bValves_Status[4]=false;
  sListadeLavados[0].bValves_Status[5]=false;
  sListadeLavados[0].bValves_Status[6]=false;
  
  //Se lee de la EEPROM el tiempo de lavado guardado - si el vaor esta fuera de rango se configura el valor por defecto
  nAuxTime=LAB_EEPROM_Read_LavTime(0);
  if(nAuxTime>=1 && nAuxTime<=MAX_LAV_TIME)
    sListadeLavados[0].uiSegundos = nAuxTime;
  else
    sListadeLavados[0].uiSegundos = DEFAULT_LAV_TIME;

  sListadeLavados[1].strLiteralDescription = "Lavado inicial con agua";
  sListadeLavados[1].uiEstado_Bomba = BOMBA_ON;
  sListadeLavados[1].bValves_Status[0]=true;
  sListadeLavados[1].bValves_Status[1]=true;
  sListadeLavados[1].bValves_Status[2]=false;
  sListadeLavados[1].bValves_Status[3]=false;
  sListadeLavados[1].bValves_Status[4]=false;
  sListadeLavados[1].bValves_Status[5]=false;
  sListadeLavados[1].bValves_Status[6]=false;
  //Se lee de la EEPROM el tiempo de lavado guardado - si el vaor esta fuera de rango se configura el valor por defecto
  nAuxTime=LAB_EEPROM_Read_LavTime(1);
  if(nAuxTime>=1 && nAuxTime<=MAX_LAV_TIME)
    sListadeLavados[1].uiSegundos = nAuxTime;
  else
    sListadeLavados[1].uiSegundos = DEFAULT_LAV_TIME;

  sListadeLavados[2].strLiteralDescription = "Desague agua con aire";
  sListadeLavados[2].uiEstado_Bomba = BOMBA_OFF;
  sListadeLavados[2].bValves_Status[0]=false;
  sListadeLavados[2].bValves_Status[1]=true;
  sListadeLavados[2].bValves_Status[2]=true;
  sListadeLavados[2].bValves_Status[3]=false;
  sListadeLavados[2].bValves_Status[4]=false;
  sListadeLavados[2].bValves_Status[5]=false;
  sListadeLavados[2].bValves_Status[6]=false;
  //Se lee de la EEPROM el tiempo de lavado guardado - si el vaor esta fuera de rango se configura el valor por defecto
  nAuxTime=LAB_EEPROM_Read_LavTime(2);
  if(nAuxTime>=1 && nAuxTime<=MAX_LAV_TIME)
    sListadeLavados[2].uiSegundos = nAuxTime;
  else
    sListadeLavados[2].uiSegundos = DEFAULT_LAV_TIME;

  sListadeLavados[3].strLiteralDescription = "Limpieza con Alcalino";
  sListadeLavados[3].uiEstado_Bomba = BOMBA_ON;
  sListadeLavados[3].bValves_Status[0]=false;
  sListadeLavados[3].bValves_Status[1]=false;
  sListadeLavados[3].bValves_Status[2]=false;
  sListadeLavados[3].bValves_Status[3]=true; //Retorno de alcalino
  sListadeLavados[3].bValves_Status[4]=false;
  sListadeLavados[3].bValves_Status[5]=false;
  sListadeLavados[3].bValves_Status[6]=true; //Toma de alcalino
  //Se lee de la EEPROM el tiempo de lavado guardado - si el vaor esta fuera de rango se configura el valor por defecto
  nAuxTime=LAB_EEPROM_Read_LavTime(3);
  if(nAuxTime>=1 && nAuxTime<=MAX_LAV_TIME)
    sListadeLavados[3].uiSegundos = nAuxTime;
  else
    sListadeLavados[3].uiSegundos = DEFAULT_LAV_TIME;

  sListadeLavados[4].strLiteralDescription = "Desague alcalino con aire";
  sListadeLavados[4].uiEstado_Bomba = BOMBA_OFF;
  sListadeLavados[4].bValves_Status[0]=false;
  sListadeLavados[4].bValves_Status[1]=false;
  sListadeLavados[4].bValves_Status[2]=true; //Entrada de aire
  sListadeLavados[4].bValves_Status[3]=false;
  sListadeLavados[4].bValves_Status[4]=true; //Retorno de alcalino
  sListadeLavados[4].bValves_Status[5]=false;
  sListadeLavados[4].bValves_Status[6]=false;
  //Se lee de la EEPROM el tiempo de lavado guardado - si el vaor esta fuera de rango se configura el valor por defecto
  nAuxTime=LAB_EEPROM_Read_LavTime(4);
  if(nAuxTime>=1 && nAuxTime<=MAX_LAV_TIME)
    sListadeLavados[4].uiSegundos = nAuxTime;
  else
    sListadeLavados[4].uiSegundos = DEFAULT_LAV_TIME;

  sListadeLavados[5].strLiteralDescription = "Lavado del alcalino con agua";
  sListadeLavados[5].uiEstado_Bomba = BOMBA_ON;
  sListadeLavados[5].bValves_Status[0]=true; //Entrada de agua
  sListadeLavados[5].bValves_Status[1]=true; //Desague
  sListadeLavados[5].bValves_Status[2]=false;
  sListadeLavados[5].bValves_Status[3]=false;
  sListadeLavados[5].bValves_Status[4]=false;
  sListadeLavados[5].bValves_Status[5]=false;
  sListadeLavados[5].bValves_Status[6]=false;
  //Se lee de la EEPROM el tiempo de lavado guardado - si el vaor esta fuera de rango se configura el valor por defecto
  nAuxTime=LAB_EEPROM_Read_LavTime(5);
  if(nAuxTime>=1 && nAuxTime<=MAX_LAV_TIME)
    sListadeLavados[5].uiSegundos = nAuxTime;
  else
    sListadeLavados[5].uiSegundos = DEFAULT_LAV_TIME;

  //strcpy(sListadeLavados[6].strLiteralDescription, "Desinfección con peracético");
  sListadeLavados[6].strLiteralDescription = "Desinfección con peracético";
  sListadeLavados[6].uiEstado_Bomba = BOMBA_ON;
  sListadeLavados[6].bValves_Status[0]=false;
  sListadeLavados[6].bValves_Status[1]=false;
  sListadeLavados[6].bValves_Status[2]=false;
  sListadeLavados[6].bValves_Status[3]=false; 
  sListadeLavados[6].bValves_Status[4]=true; //Retorno Peracetico
  sListadeLavados[6].bValves_Status[5]=false;
  sListadeLavados[6].bValves_Status[6]=true; //Entrada de Peracetico
  //Se lee de la EEPROM el tiempo de lavado guardado - si el vaor esta fuera de rango se configura el valor por defecto
  nAuxTime=LAB_EEPROM_Read_LavTime(6);
  if(nAuxTime>=1 && nAuxTime<=MAX_LAV_TIME)
    sListadeLavados[6].uiSegundos = nAuxTime;
  else
    sListadeLavados[6].uiSegundos = DEFAULT_LAV_TIME;

  sListadeLavados[7].strLiteralDescription = "Desagote del peracético con aire";
  sListadeLavados[7].uiEstado_Bomba = BOMBA_OFF;
  sListadeLavados[7].bValves_Status[0]=false;
  sListadeLavados[7].bValves_Status[1]=false;
  sListadeLavados[7].bValves_Status[2]=true; //innyector de aire
  sListadeLavados[7].bValves_Status[3]=false; 
  sListadeLavados[7].bValves_Status[4]=true; //Retorno Peracetico
  sListadeLavados[7].bValves_Status[5]=false;
  sListadeLavados[7].bValves_Status[6]=false; 
  //Se lee de la EEPROM el tiempo de lavado guardado - si el vaor esta fuera de rango se configura el valor por defecto
  nAuxTime=LAB_EEPROM_Read_LavTime(7);
  if(nAuxTime>=1 && nAuxTime<=MAX_LAV_TIME)
    sListadeLavados[7].uiSegundos = nAuxTime;
  else
    sListadeLavados[7].uiSegundos = DEFAULT_LAV_TIME;


}
/*****************************************
  Configura la secuencia de lavado completo
******************************************/
void ConfigLavadoCompleto(void)
{
  int i;
  nMaxExecution_steps = 8;
  for(i=0; i<nMaxExecution_steps; i++)
                       nListaPasos[i] = i;
  nCurrent_step = 0;
  nLavStatus=LAV_INIT;
}

void Config_Lav_Time(int nLav, int nTimeSeconds)
{
  if(nLav >=0 && nLav < MAX_NUM_LAVADOS)
       sListadeLavados[nLav].uiSegundos = nTimeSeconds; 
}

const unsigned int MAX_MESSAGE_LENGTH = 30;

/*****************************************
* //Tarea para comunicación serie - monitorea recepcion de comandos por el puerto serie
***********************************************/
void Task_SerialRead(void *pvParameters)
{
  int SerialBufferCount=0;
  int i, nAuxVal, nAuxVal1;
  String text, auxtext, auxtext1;
  char sCommand[MAX_MESSAGE_LENGTH];

  for (;;)
  {
    SerialBufferCount = Serial.available();
    if (SerialBufferCount > 0)
    {
        for (i = 0; (i < SerialBufferCount)  &&  (i< MAX_MESSAGE_LENGTH); i++)
        {
          sCommand[i] = Serial.read();
        }
        for (i = 0; (i < SerialBufferCount)  &&  (i < MAX_MESSAGE_LENGTH); i++)
        {
          Serial.print("("+(String)i + ":" + (String)sCommand[i]+") - ");
        }

        Serial.println("\nBuffer size:" + (String) SerialBufferCount);
        text=sCommand;

        Serial.println("Comando: "+ text);
        if (text.startsWith("start"))
        {
          ConfigLavadoCompleto();
          Serial.println("Iniciando Lavado completo");
        }
        if (text.startsWith("inilav:"))
        {
          auxtext =text.substring(7,8);
          nAuxVal =  auxtext.toInt();
          if(nAuxVal>=0 && nAuxVal<MAX_NUM_LAVADOS)
          {
            Serial.println("Iniciando Lavado #: " + auxtext );
            nMaxExecution_steps = 1;
            nListaPasos[0] = nAuxVal;
            nCurrent_step = 0;
            nLavStatus=LAV_INIT;
          }
        }

        if (text.startsWith("config:"))
        {
          
          auxtext =text.substring(7,8);
          nAuxVal =  auxtext.toInt();
          auxtext1 =text.substring(9);
          nAuxVal1 =  auxtext1.toInt();

          if(nAuxVal>=0 && nAuxVal<MAX_NUM_LAVADOS && nAuxVal1>=1 && nAuxVal1<10000)
          {
            sListadeLavados[nAuxVal].uiSegundos = nAuxVal1;
            Serial.println("Configurando Lavado #: " + auxtext );
            Serial.println("Tiempo (seg): " + auxtext1);
          }
          else
          {
            Serial.println("Error en Parametros " );
            Serial.println("Lavado #: " + auxtext );
            Serial.println("Tiempo (seg): " + auxtext1);
          }
        }
        if (text.startsWith("pumpon"))
        {
          pump_on();
        }
        if (text.startsWith("pumpoff"))
        {
          pump_off();
        }
        if (text.startsWith("valvopen:"))
        {
          auxtext =text.substring(9);
          nAuxVal =  auxtext.toInt();
          if(nAuxVal>=0 && nAuxVal< MAX_NUM_VALVES)
          {
            Valve_Open(nAuxVal);
            Serial.println("Abriendo Valvula #: " + (String) nAuxVal);
          }
          else
            Serial.println("Error en parametro #: " + (String) nAuxVal);
        }
        if (text.startsWith("valvclose:"))
        {
          auxtext = text.substring(10);
          nAuxVal =  auxtext.toInt();
          if(nAuxVal >= 0 && nAuxVal < MAX_NUM_VALVES)
          {
            Valve_Close(nAuxVal);
            Serial.println("Cerrando Valvula #: " + (String) nAuxVal);
          }
          else
            Serial.println("Error en parametro #: " + (String) nAuxVal);

        }
        if (text.startsWith("stop"))
      {
        nLavStatus = LAV_STAND_BY;
        //Se apagan la bomba y se cierran todas las valvulas
        pump_off();
        for(i=0; i<MAX_NUM_VALVES; i++)
                              Valve_Close(i);
      }
        if (text.startsWith("EEPROM"))
      {
        for (i=0;i<EEPROM_SIZE;i++)
        {
          nAuxVal = EEPROM.read(i);
          Serial.print("Address:" +(String) i);
          Serial.print(" - 0x");
          Serial.print(nAuxVal < 16 ? "0" : "");
          Serial.print(nAuxVal, HEX);
          Serial.println();
        }
      }
        if (text.startsWith("Grabar"))
        {
          for(i=0;i<MAX_NUM_LAVADOS;i++)
          {
            LAB_EEPROM_Save_LavTime(i,sListadeLavados[i].uiSegundos);
          }

        }
        if (text.startsWith("Lavlist"))
        {
          for (i=0;i<MAX_NUM_LAVADOS;i++)
          {
            Serial.println("Lavado:" + (String) i +" - "+ sListadeLavados[i].strLiteralDescription );
            Serial.println("Duración (seg):" + (String) sListadeLavados[i].uiSegundos);
            Serial.println("-----------");
          }
        }

     }
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }

}


//Maneja la ejecucion de pasos de lista de ejecucion de pasos
void Task_control_lavado(void *pvParameters)
{
  int i;
  volatile int nTaskdelay = 1000;
  volatile int nTiempo;
  for(;;)
  {
    switch (nLavStatus)
    {
      case LAV_INIT:
          if(nCurrent_step>=0)
          {
            if(nCurrent_step >= nMaxExecution_steps)
            {
              nLavStatus = LAV_STAND_BY;
              //Se apagan la bomba y se ceirran todas las valvulas
              pump_off();
              for(i=0; i<MAX_NUM_VALVES; i++)
                                    Valve_Close(i);

            }
            else
            {
          //    if (serial_on)
                     Serial.println("Lavado: " + (String) sListadeLavados[nListaPasos[nCurrent_step]].strLiteralDescription);
                     Serial.println("Nro de Lavado:" + (String) nListaPasos[nCurrent_step]);
                     Serial.println("Paso:" + (String) nCurrent_step);
 
              for(i=0; i<MAX_NUM_VALVES; i++)
                if (sListadeLavados[nListaPasos[nCurrent_step]].bValves_Status[i] == true)
                {
                  Valve_Open(i);
                  if (serial_on)
                     Serial.println("Apertura de valvula: " + (String) (i+1));
                }
                else
                    Valve_Close(i);


              if(sListadeLavados[nListaPasos[nCurrent_step]].uiEstado_Bomba == BOMBA_ON) 
              {
                if (serial_on)
                     Serial.println("Encendido de bomba");
                pump_on();
              }
              else
              {
                if (serial_on)
                     Serial.println("Apagado de bomba");
                pump_off();
              }
              nLavStatus = LAV_TIME_CTRL;
              nTiempo = 0;
             }
          }
      break;

      case LAV_TIME_CTRL:
        nTiempo++;
        //if (serial_on)
                Serial.println("Seconds: " +  (String) nTiempo  +  "/" +  (String) sListadeLavados[nListaPasos[nCurrent_step]].uiSegundos);
        //Si se cumple el tiempo del lavado se pasa a inicializar el siguiente
        if (nTiempo >= sListadeLavados[nListaPasos[nCurrent_step]].uiSegundos) 
        {
          nCurrent_step++;
          nLavStatus=LAV_INIT;
          nTiempo =0 ;
        }
      break;

      case LAV_STAND_BY:
          nCurrent_step = -1;
      break;
  
      default:
      break;
    }

    vTaskDelay(nTaskdelay / portTICK_PERIOD_MS );
  }
}
//------------------------------------------------------------------------------------------------------------------------------
//Tarea para verificar y conectar a wifi & telegram ----------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

#define WIFI_ST_MACHINE_BEGIN 0
#define WIFI_ST_MACHINE_WAIT 1
#define WIFI_ST_MACHINE_RECONNECT 2
#define WIFI_ST_MACHINE_CONNECTED 3
volatile int nWifiStatMachine=0;
String sWiFiprovider = "avc168583";
String sWiFipasswd = "23445020";

void Task_Wifi_Connect(void *pvParameters)
{
  int nConnectionTimeout=0;
  volatile int nTaskdelay = 1000;

  for (;;)
  {
    switch (nWifiStatMachine)
    {
      //Primera conexión utiliación de la funcion begin
     case WIFI_ST_MACHINE_BEGIN:
      //Inicializacion primera conexión
      WiFi.mode(WIFI_STA);
      secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
      WiFi.begin("MovistarFibra-B29558", "emcsm5+2211");
      //WiFi.begin(sWiFiprovider, sWiFipasswd);
      nWifiStatMachine = WIFI_ST_MACHINE_WAIT;
      nConnectionTimeout=0;
      Serial.println("Task_Wifi_Connect - Connecting to : " + sWiFiprovider);
     break;
     //Espera para verificar la conexion
     case WIFI_ST_MACHINE_WAIT:
         if (WiFi.status() != WL_CONNECTED)
         {
          Serial.println("Task_Wifi_Connect - Waiting to establish the connection : " + (String) nConnectionTimeout + "/30");
          nConnectionTimeout++;
          //espero 30 segundos ya que la tarea corre 1 vez por segundo
          if (nConnectionTimeout>=30)
          {
            nWifiStatMachine = WIFI_ST_MACHINE_RECONNECT;
            Serial.println("Task_Wifi_Connect - Connection Timeout trying to reconnect");
          }
         }
         else
         {
           bot.longPoll = 30;
           nWifiStatMachine = WIFI_ST_MACHINE_CONNECTED;
           Serial.print("\nTask_Wifi_Connect -WiFi connected. IP address: ");
           Serial.println(WiFi.localIP());
         }
        
    break;
    case WIFI_ST_MACHINE_RECONNECT:
        if ((WiFi.status() != WL_CONNECTED) )
        {
          Serial.println("Task_Wifi_Connect - Reconnecting to WiFi...");
          WiFi.disconnect();
          WiFi.reconnect();
          nWifiStatMachine = WIFI_ST_MACHINE_WAIT;
          nConnectionTimeout = 0;
        }
        else
        {
            nWifiStatMachine = WIFI_ST_MACHINE_CONNECTED;
            Serial.print("\nTask_Wifi_Connect -WiFi connected. IP address: ");
            Serial.println(WiFi.localIP());
        }

    break;

    case WIFI_ST_MACHINE_CONNECTED:
        nTaskdelay=3000; //Se cambia la velocidad de muestreo a 3 segundos
        if ((WiFi.status() != WL_CONNECTED) )
        {
          Serial.print("\nTask_Wifi_Connect - Connection lost - WiFi diconnected - trying to reconnect");
          nWifiStatMachine = WIFI_ST_MACHINE_RECONNECT;
          nTaskdelay=1000;
        }
        // print the received signal strength:
        Serial.print("Task_Wifi_Connect - Signal strength (RSSI):");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm"); 
    break;
    
    default:
      break;
    }

    vTaskDelay( nTaskdelay / portTICK_PERIOD_MS );
  }
}
String sLavarkeyboardJson;
void IniBotKeyboard_LavarOptions(void)
{
  sLavarkeyboardJson = "[";
  sLavarkeyboardJson +="[{ \"text\" : \"Lavado Completo\", \"callback_data\" : \"inilav\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Descarga Residual de cerveza\", \"callback_data\" : \"inilav1\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Lavado inicial con agua\", \"callback_data\" : \"inilav2\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Desague de agua con aire\", \"callback_data\" : \"inilav3\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Limpieza con Alcalino\", \"callback_data\" : \"inilav4\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Desague alcalino con aire\", \"callback_data\" : \"inilav5\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Lavado del alcalino con agua\", \"callback_data\" : \"inilav6\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Desinfección con peracético\", \"callback_data\" : \"inilav7\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Desague del peracético con aire\", \"callback_data\" : \"inilav8\" }],";
  sLavarkeyboardJson +="[{ \"text\" : \"Stop\", \"callback_data\" : \"stop\" }]";
  sLavarkeyboardJson +="]";
}


String sManualkeyboardJson;
void IniBotKeyboard_ManualOptions(void)
{
  sManualkeyboardJson = "[";
  sManualkeyboardJson +="[{ \"text\" : \"Encender Bomba\", \"callback_data\" : \"pumpon\" }],";
  sManualkeyboardJson +="[{ \"text\" : \"Apagar Bomba\", \"callback_data\" : \"pumpoff\" }]";
  sManualkeyboardJson +="]";
}

void setup() 
{
 LAB_EEPROM_Init();
 IniBotKeyboard_ManualOptions();
 IniBotKeyboard_LavarOptions();
 Serial.begin(115200);
 Serial.println();
 dout_setup();
 ConfigListadeLavados();
 delay(500);
 xTaskCreatePinnedToCore(Task_control_lavado, "Control secuencia lavado", 10000, NULL, 1, &Task1, 1);
 delay(500);
 xTaskCreatePinnedToCore(Task_SerialRead, "Serial Comands", 20000, NULL, 1, &Task2, 1);
 delay(500);
 xTaskCreatePinnedToCore(Task_Wifi_Connect, "Wifi", 20000, NULL, 1, &Task3, 0);
 delay(500);

}

void loop() 
{
  int i;
  int j;
  int numNewMessages=0;
  String chat_id;
  String text;
  String from_name;
  String sReturnMessage ="";

//  char szAuxStr[16];
//  char szAuxStr2[16];
  // esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts

   if ((WiFi.status() == WL_CONNECTED) && (nWifiStatMachine==WIFI_ST_MACHINE_CONNECTED))
   {
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    for (i = 0; i < numNewMessages; i++)
    {
      chat_id = bot.messages[i].chat_id;
      text = bot.messages[i].text;
      from_name = bot.messages[i].from_name;
      Serial.println("\nProcessing Message :" + (String) i + " - From Name: "+(String) from_name + " - Msg: "+ text);
      if (text == "/start")
      {
        sReturnMessage = "Hola, " + from_name + ".\n";
        sReturnMessage += "Soy el robot que lava de Barriles mis comandos basicos son: \n\n";
        sReturnMessage += "/manualoptions : Manual Commands\n";
        //sReturnMessage += "/config : Configura Tiempos de lavado\n";
        sReturnMessage += "/status : Muestra el estado del sistema\n";
        sReturnMessage += "/lavar : inicia lavados\n";
        bot.sendMessage(chat_id, sReturnMessage, "Markdown");
      }
      if (text == "/manualoptions")
        bot.sendMessageWithInlineKeyboard(chat_id, "Selecciona las siguientes opciones", "", sManualkeyboardJson);

      if (text == "/lavar")
        bot.sendMessageWithInlineKeyboard(chat_id, "Selecciona las siguientes opciones", "", sLavarkeyboardJson);

      if (text == "/status")
      {
        if(nLavStatus != LAV_STAND_BY)
        {
          sReturnMessage = "Estoy lavando\n";
          sReturnMessage += "Lavado: "+ sListadeLavados[nListaPasos[nCurrent_step]].strLiteralDescription + "\n";
        }
        else
          sReturnMessage = "Esperando indicaciones";

        bot.sendMessage(chat_id, sReturnMessage, "Markdown");

      }


      if (text == "pumpon")
      {
        pump_on();
        //digitalWrite(Pump_pin, PUMP_ON); 
        bot.sendMessage(chat_id, "Bomba Encendida", "");
      }

      if (text == "pumpoff")
      {
        pump_off();
        //digitalWrite(Pump_pin, PUMP_OFF); 
        bot.sendMessage(chat_id, "Bomba Apagada", "");
      }
      
      if (text == "inilav")
      {
        ConfigLavadoCompleto();
        bot.sendMessage(chat_id, "Iniciando Lavado completo", "");
      }
      if (text == "inilav1")
      {
        nMaxExecution_steps = 1;
        nListaPasos[0] = 0;
        nCurrent_step = 0;
        nLavStatus=LAV_INIT;
        sReturnMessage="Iniciando Lavado: " + sListadeLavados[nListaPasos[0]].strLiteralDescription;
        bot.sendMessage(chat_id, sReturnMessage, "");
      }

      if (text == "inilav2")
      {
        nMaxExecution_steps = 1;
        nListaPasos[0] = 1;
        nCurrent_step = 0;
        nLavStatus=LAV_INIT;
        sReturnMessage="Iniciando Lavado: " + sListadeLavados[nListaPasos[0]].strLiteralDescription;
        bot.sendMessage(chat_id, sReturnMessage, "");
      }

      if (text == "inilav3")
      {
        nMaxExecution_steps = 1;
        nListaPasos[0] = 2;
        nCurrent_step = 0;
        nLavStatus=LAV_INIT;
        sReturnMessage="Iniciando Lavado: " + sListadeLavados[nListaPasos[0]].strLiteralDescription;
        bot.sendMessage(chat_id, sReturnMessage, "");
      }

      if (text == "inilav4")
      {
        nMaxExecution_steps = 1;
        nListaPasos[0] = 3;
        nCurrent_step = 0;
        nLavStatus=LAV_INIT;
        sReturnMessage="Iniciando Lavado: " + sListadeLavados[nListaPasos[0]].strLiteralDescription;
        bot.sendMessage(chat_id, sReturnMessage, "");
      }

      if (text == "inilav5")
      {
        nMaxExecution_steps = 1;
        nListaPasos[0] = 4;
        nCurrent_step = 0;
        nLavStatus=LAV_INIT;
        sReturnMessage="Iniciando Lavado: " + sListadeLavados[nListaPasos[0]].strLiteralDescription;
        bot.sendMessage(chat_id, sReturnMessage, "");
      }

      if (text == "inilav6")
      {
        nMaxExecution_steps = 1;
        nListaPasos[0] = 5;
        nCurrent_step = 0;
        nLavStatus=LAV_INIT;
        sReturnMessage="Iniciando Lavado: " + sListadeLavados[nListaPasos[0]].strLiteralDescription;
        bot.sendMessage(chat_id, sReturnMessage, "");
      }

      if (text == "inilav7")
      {
        nMaxExecution_steps = 1;
        nListaPasos[0] = 6;
        nCurrent_step = 0;
        nLavStatus=LAV_INIT;
        sReturnMessage="Iniciando Lavado: " + sListadeLavados[nListaPasos[0]].strLiteralDescription;
        bot.sendMessage(chat_id, sReturnMessage, "");
      }

      if (text == "inilav8")
      {
        nMaxExecution_steps = 1;
        nListaPasos[0] = 7;
        nCurrent_step = 0;
        nLavStatus=LAV_INIT;
        sReturnMessage="Iniciando Lavado: " + sListadeLavados[nListaPasos[0]].strLiteralDescription;
        bot.sendMessage(chat_id, sReturnMessage, "");
      }
      if (text == "stop")
      {
        //Se apagan la bomba y se ceirran todas las valvulas
        pump_off();
        for(j=0; j<MAX_NUM_VALVES; j++)
                              Valve_Close(j);
        sReturnMessage="Apagado y cierre de válvulas";
        bot.sendMessage(chat_id, sReturnMessage, "");
      }


    }
   }
  delay(5000);

}
