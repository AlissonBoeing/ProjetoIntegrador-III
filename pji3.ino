#include "EEPROM.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "time.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#ifdef ESP32
  #include <WiFi.h>
  #include <HTTPClient.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
#endif
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>




// --- Definições do BLE ---- //
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};





// Definições EEPROM
EEPROMClass TEMP("eeprom0", 512);
EEPROMClass HUMI("eeprom1", 512);
EEPROMClass TIME("eeprom2", 512);
int cont = 0;
int tmp = 0;
int hum = 0;
int tim = 0;
float T_TEMP;
float T_HUMI;
char  T_TIME[64];

int flag_ble = 0;

// Definições wifi
const char* ssid     = "AlissonBoeing";
const char* password = "alisson123";



// Definições NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


// Definições (estrutura alternativa de data para quando estiver offline)
struct tm data;
String formattedDate;
String dayStamp;
String timeStamp;


// Definições servidor web
const char* serverName = "http://projetodoalisson.serverdo.in/post-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";


// Definições sensor
const int oneWireBus = 4;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }

        Serial.println();

        // Do stuff based on the command received from the app
        if (rxValue.find("A") != -1) { 
  
          Serial.print("Turning ON!");
          digitalWrite(LED, HIGH);
        }
        else if (rxValue.find("B") != -1) {
          Serial.print("Turning OFF!");
          digitalWrite(LED, LOW);
        }

        Serial.println();
        Serial.println("*********");
      }
    }
};

void setup() {
  Serial.begin(115200);

  // ---------------- BLE --------------------//

  pinMode(LED, OUTPUT);

  // Create the BLE Device
  BLEDevice::init("ESP32 BLE Sementes"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify..."); 



  // --------------------------------------//

// ----- EEPROM ----- ///

  if (!TEMP.begin(TEMP.length())) {
    Serial.println("Failed to initialise TEMP");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!HUMI.begin(HUMI.length())) {
    Serial.println("Failed to initialise HUMII");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!TIME.begin(TIME.length())) {
    Serial.println("Failed to initialise TIME");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  
    TEMP.put(cont, 0);
    HUMI.put(cont, 0);
    TIME.put(cont, '\0');
   // ---------- ///


  timeval tv;//Cria a estrutura temporaria para funcao abaixo.
  tv.tv_sec = 1603162860;//Atribui minha data atual. 
  settimeofday(&tv, NULL);//Configura o RTC para manter a data atribuida atualizada.


 timeClient.setTimeOffset(-10800);
  
  sensors.begin();
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  
  int j = 0;
  
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print("tentando reconectar");
    j = j + 1;
    if ( j == 10 ) {
      break;
    }
  }
  
  
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

}




void loop() {

    sensors.requestTemperatures(); 
    float temperatureC = sensors.getTempCByIndex(0);
    Serial.print(temperatureC);
    timeClient.update();

  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
  
//--------------------------------------------////

    vTaskDelay(pdMS_TO_TICKS(1000));//Espera 1 seg
    //time_t tt = 1603159920 + 1;//Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
    time_t tt = time(NULL);
    data = *gmtime(&tt);//Converte o tempo atual e atribui na estrutura
    
    char data_formatada[64];

    strftime(data_formatada, 64, "%Y-%m-%dT%H:%M:%SZ", &data);//Cria uma String formatada da estrutura "data"
    printf("\nUnix Time: %d\n", int32_t(tt));//Mostra na Serial o Unix time
    printf("Data formatada: %s\n", data_formatada);//Mostra na Serial a data formatada

//-------------------------------------------/////


/// --------------------------------////

    if (deviceConnected) {
    Serial.println("CONECTOU BLEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
    if (cont == 0) {
      Serial.println("EEPROM VAZIA");
    } else {
      tmp = 0;
      hum = 0;
      tim = 0;
      Serial.println("EEPROM contém " + String(cont) + " dados");
      while (cont != 0) {
            TEMP.get(tmp, T_TEMP);
            HUMI.get(hum, T_HUMI);
            TIME.get(tim, T_TIME);

            Serial.print("enviando temp via BLE: ");   Serial.println(T_TEMP);
            Serial.print("enviando humi via BLE: "); Serial.println(T_HUMI);
            Serial.print("enviando time: ");    Serial.println(T_TIME);

            // Prepare your HTTP POST request data
            String httpRequestData = "api_key=" + apiKeyValue + "&value1=" + String(T_TEMP)
                                   + "&value2=" + String(T_HUMI) + "&value3=" + String(T_TIME) + "";
            char tx[50];
            httpRequestData.toCharArray(tx, 50);
            pCharacteristic->setValue(tx);
    
            pCharacteristic->notify(); // Send the value to the app!
            Serial.print("*** Sent Value: ");
            Serial.print(httpRequestData);
            Serial.println(" ***");

             TEMP.put(cont, 0);
        HUMI.put(cont, 0);
        TIME.put(cont, '\0');
        tmp = tmp + 4;
        hum = hum + 4;
        tim = tim + 64;
        cont = cont - 1;
        
      }
      digitalWrite(LED, LOW);
      cont = 0;
      tmp = 0;
      hum = 0;
      tim = 0;
      }

  } else {
    Serial.println("Sem dispositivos conectados");
  }



////-----------------------------------////


  //Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
  
    HTTPClient http;
  // Your Domain name with URL path or IP address with path
    http.begin(serverName);
        // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
     
    if (cont == 0) {
      Serial.println("EEPROM VAZIA");
    } else {
      tmp = 0;
      hum = 0;
      tim = 0;
      Serial.println("EEPROM contém " + String(cont) + " dados");
      while (cont != 0) {
            TEMP.get(tmp, T_TEMP);
            HUMI.get(hum, T_HUMI);
            TIME.get(tim, T_TIME);
            Serial.print("enviando temp: ");   Serial.println(T_TEMP);
            Serial.print("enviando humi: "); Serial.println(T_HUMI);
            Serial.print("enviando time: ");    Serial.println(T_TIME);

            // Prepare your HTTP POST request data
            String httpRequestData = "api_key=" + apiKeyValue + "&value1=" + String(T_TEMP)
                                   + "&value2=" + String(T_HUMI) + "&value3=" + String(T_TIME) + "";
            Serial.print("httpRequestData: ");
            Serial.println(httpRequestData);
            
            // Send HTTP POST request
            int httpResponseCode = http.POST(httpRequestData);

            if (httpResponseCode>0) {
              Serial.print("HTTP Response code: ");
              Serial.println(httpResponseCode);
            }
            else {
              Serial.print("Error code: ");
              Serial.println(httpResponseCode);
            }

        TEMP.put(cont, 0);
        HUMI.put(cont, 0);
        TIME.put(cont, '\0');
        tmp = tmp + 4;
        hum = hum + 4;
        tim = tim + 64;
        cont = cont - 1;
        
      }
      cont = 0;
      tmp = 0;
      hum = 0;
      tim = 0;
    }
     
    // Prepare your HTTP POST request data
    String httpRequestData = "api_key=" + apiKeyValue + "&value1=" + String(temperatureC)
                           + "&value2=" + String(temperatureC) + "&value3=" + String(formattedDate) + "";
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);
    
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
     
   
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi desconectado");
    Serial.println("Armazenando temperatura " + String(temperatureC) + " nas eeprom's na posicao " + String(cont) + " tempo " + String(data_formatada));

    
    TEMP.put(tmp, temperatureC);
    HUMI.put(hum, temperatureC);
    TIME.put(tim, data_formatada);
    Serial.println("Armazenado");
    
    cont = cont + 1;
    tmp = tmp + 4;
    hum = hum + 4;
    tim = tim + 64;
    int j = 0;
     
    while(WiFi.status() != WL_CONNECTED) { 
      delay(500);
      Serial.println("tentando reconectar");
      j = j + 1;

      if ( j == 10 ) {
        break;
      }
    
    }
    
  }

  //Send an HTTP POST request every 30 seconds
  delay(5000);  
}
