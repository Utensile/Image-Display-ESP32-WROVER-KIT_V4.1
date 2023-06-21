#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include "FS.h"
#include "Adafruit_GFX.h"
#include "WROVER_KIT_LCD.h"
#include <TFT_eSPI.h>
#define Bu1 15
#define Bu2 22
#define fr 36
#define X_AXIS_PIN 39
#define Y_AXIS_PIN 35
int t=0;
int xdir=0;
int ydir=0;
/*


void setup() {
  Serial.begin(115200);
  
}

void loop() {
  //print the buttons when their state changes
  
}*/

#define MAXimg 20


long last_time = 0;
bool display=false;

const char* ssid = "RE:Lab";
const char* password = "Interact2019!";

char* imagePath[MAXimg] = {"/zanas.jpg", "/mario.jpg", "/beacon.jpg", "/black_hole.jpg", "/earth.jpg", "/orbit.jpg", "/rocket.jpg", "/rover.jpg"};
char* imgNameChar[MAXimg];

String imgName[MAXimg];
const int w = 320;
const int h = 240;
int imgN = 8;
int pos = 0;
int nUpload = 0;
bool inizio = true;
WROVER_KIT_LCD tft;

AsyncWebServer server(80);

void configura_wifi(const char* ssid, const char* password){
  display = false;
  if(WiFi.status() == WL_CONNECTED){
    WiFi.disconnect();
    delay(100);
  }
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tft.setCursor(w/2-70, h/2-15);
    tft.setTextColor(WROVER_BLACK);
    tft.setTextSize(1);
    tft.println("Connettendomi alla rete...");
    // Serial.println("Connessione in corso alla rete WiFi...");
  }

  Serial.println("Connessione WiFi stabilita");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());
  tft.fillScreen(WROVER_NAVY);
  tft.setCursor(w/2-90, h/2-30);
  tft.setTextSize(2);
  tft.setTextColor(WROVER_WHITE);
  tft.println("digita su Google: ");
  tft.setCursor(w/2-120, h/2);
  tft.setTextSize(3);
  tft.println(WiFi.localIP());
  tft.setTextColor(WROVER_WHITE);
  delay(2000);



}



void removeElement(char* arr[], int& size, int index) {
  if (index < 0 || index >= size) {
    // Invalid index, do nothing
    return;
  }
  // Shift elements after the index
  for (int i = index; i < size - 1; i++) {
    arr[i] = arr[i + 1];
  }
  // Reduce the size of the array
  size--;
}




void setup(){
  Serial.begin(115200);
  pinMode(Bu1, INPUT_PULLUP);
  pinMode(Bu2, INPUT_PULLUP);
  // Montaggio della memoria SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Errore nel montaggio di SPIFFS");
    return;
  }
  //inizializzazione canvas
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(WROVER_WHITE);

  // Connessione alla rete WiFi

  configura_wifi(ssid, password);





  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    // Stampa il nome di ogni file nella directory radice
    if (file.name() != "") {
      Serial.println(file.name());
      file = root.openNextFile();
    }
    else {
      Serial.println("Problema");
      break;
    }
  }
  root.close();

  

  // Configurazione del server web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/index.html", "text/html");
    tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    display=true;

  });

  server.on("/H", HTTP_GET, [](AsyncWebServerRequest* request) {
    pos = (pos + 1) % imgN;
    request->send(SPIFFS, "/index.html", "text/html");
    tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    display=true;
  });
  server.on("/L", HTTP_GET, [](AsyncWebServerRequest* request) {
    pos -= 1;
    if (pos < 0)
      pos = imgN - 1;
    request->send(SPIFFS, "/index.html", "text/html");
    tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    display=true;
  });

  server.on("/image.jpg", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, imagePath[pos], "image/jpeg");
  });



  server.on("/D", HTTP_GET, [](AsyncWebServerRequest* request) {
    tft.fillScreen(WROVER_RED);
    tft.setCursor(w/2-80, h/2-15);
    tft.setTextColor(WROVER_WHITE);  
    tft.setTextSize(2);
    tft.println("Image Deleted");
    int size = sizeof(imagePath) / sizeof(imagePath[0]);
    int size2 = sizeof(imgName) / sizeof(imgName[0]);
    removeElement(imagePath, size, pos);
    removeElement(imgNameChar, size2, pos);
    delay(500);
    tft.drawJpgFile(SPIFFS, imagePath[pos], 0,0);

    //imgN -= 1;
    //pos += 1;
    //request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/wifi.html", "text/html");
  });

  server.on("/formdata", HTTP_GET, [](AsyncWebServerRequest *request){
    String SSID = request->getParam("SSID")->value();
    String Pass = request->getParam("Password")->value();
    
    Serial.print("SSID: ");
    Serial.println(SSID);
    
    Serial.print("Password: ");
    Serial.println(Pass);


    configura_wifi(SSID.c_str(), Pass.c_str());
    request->send(SPIFFS, "/index.html", "text/html");

  });
  // Gestione della richiesta di caricamento immagine
  server.on(
    "/upload",
    HTTP_POST,
    [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "Caricamento immagine completato");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    },
    [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {

      if (!index) {
        imgName[nUpload] = "/img" + String(nUpload) + ".jpg";
        imgNameChar[nUpload] = new char[imgName[nUpload].length() + 1];
        strcpy(imgNameChar[nUpload], imgName[nUpload].c_str());
        Serial.println(imgNameChar[nUpload]);
        File file = SPIFFS.open(imgNameChar[nUpload], FILE_WRITE);
        if (!file) {
          Serial.println("Errore nell'apertura del file");
          delete[] imgNameChar[nUpload];
          return;
        }
        file.write(data, len);
        file.close();
      }
      else {
        // Caricamento successivo del file
        File file = SPIFFS.open(imgNameChar[nUpload], FILE_APPEND);
        if (!file) {
          Serial.println("Errore nell'apertura del file");
          delete[] imgNameChar[nUpload];
          return;
        }
        file.write(data, len);
        file.close();
      }
      if (final) {
        Serial.println("Caricamento immagine completato");
        tft.drawJpgFile(SPIFFS, imgNameChar[nUpload], 0, 0);
        imgN += 1;
        pos = imgN - 1; //Metti ultimo num array
        imagePath[pos] = imgNameChar[nUpload];
        nUpload += 1;
      }
    }
  );

  // Inizializzazione del server
  server.begin();
}

void loop() {
    int luce = analogRead(fr);
    analogWrite(5, map(luce, 0, 4095, 245,0));
    delay(100);
    /*Serial.print("luce: ");
    Serial.print(luce);
    Serial.print(" | ");
    Serial.println(map(luce, 0, 4095, 245,00));*/
  
  
   int x = map(analogRead(X_AXIS_PIN), 0, 4095, 0, 100);
  int y = map(analogRead(Y_AXIS_PIN), 0, 4095, 0, 100);


  if(millis()-t>500){
    if(x==100 && xdir!=1){
      Serial.println("Right");
      xdir=1;
      t=millis();
      if (display){
        pos = (pos+1)%imgN;
      }
      display = true;
      tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    }
    else if (x==0 && xdir!=-1)
    {
      Serial.println("Left");
      xdir=-1;
      t=millis();
      if (display){
        pos -= 1;
        
        if(pos < 0){
          pos = imgN - 1;
        }
      }
      display = true;
      tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    }
    else{
      xdir=0;
    }
    if(y==100 && ydir!=1){
      Serial.println("Up");
      ydir=1;
      t=millis();
      if (display){
        pos = (pos+1)%imgN;  
      }
      display = true;
      tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    }
    else if (y==0 && ydir!=-1)
    {
      Serial.println("Down");
      ydir=-1;
      t=millis();
      if (display){
        pos -= 1;
        if(pos < 0){
          pos = imgN - 1;
        }
      }
      display = true;
      tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    }
    else{
      ydir=0;
    }
  }
  



}

void modify_brightness(int light){

  analogWrite(5, map(light, 0, 255, 100,10));
}