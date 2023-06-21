#include <WiFi.h> 
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include "FS.h"
#include "Adafruit_GFX.h"
#include "WROVER_KIT_LCD.h"
#include <TFT_eSPI.h>

#define fr 36         //Photoresistor pin
#define X_AXIS_PIN 39 //joytick module X axis pin
#define Y_AXIS_PIN 35 //joystick module Y axis pin
#define MAXimg 20     //max number of images on the ESP32

int t=0;              //time variable for debounce
int xdir=0;           //direction variable for X axis
int ydir=0;           //direction variable for Y axis
int imgN = 8;         //number of images on the ESP32
int pos = 0;          //position of the current image in the array
int nUpload = 0;      //number of uploaded images
const int w = 320;    //width of the screen
const int h = 240;    //height of the screen
bool inizio = true;   //variable to check if the ESP32 is starting
bool display=false;   //variable to check if an image is displayed on the screen

const char* ssid = "yourSSID";
const char* password = "yourPassword";

//Preloaded images initialization
char* imagePath[MAXimg] = {"/zanas.jpg", "/mario.jpg", "/beacon.jpg", "/black_hole.jpg", "/earth.jpg", "/orbit.jpg", "/rocket.jpg", "/rover.jpg"};
char* imgNameChar[MAXimg];
String imgName[MAXimg];

WROVER_KIT_LCD tft;

AsyncWebServer server(80);

// Function to connect to the specified WiFi network & change it if already connected
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

  //Serial.println("Connessione WiFi stabilita");
  //Serial.print("Indirizzo IP: ");
  //Serial.println(WiFi.localIP());

  //Print the IP address on the screen
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

  // SPIFFS initialization
  if (!SPIFFS.begin()) {
    Serial.println("Errore nel montaggio di SPIFFS");
    return;
  }

  //Display initialization
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(WROVER_WHITE);

  // WiFi connection
  configura_wifi(ssid, password);

  // Web Server configuration
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    // If the User is in the home page, the ESP sends the webpage and displays the first image
    request->send(SPIFFS, "/index.html", "text/html");
    tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    display=true;

  });

  server.on("/H", HTTP_GET, [](AsyncWebServerRequest* request) {
    // If the User pressed on the right arrow button, the ESP goes to the next image
    pos = (pos + 1) % imgN;
    request->send(SPIFFS, "/index.html", "text/html");
    tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    display=true;
  });
  server.on("/L", HTTP_GET, [](AsyncWebServerRequest* request) {
    // If the User pressed on the left arrow button, the ESP goes to the previous image
    pos -= 1;
    if (pos < 0)
      pos = imgN - 1;
    request->send(SPIFFS, "/index.html", "text/html");
    tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    display=true;
  });

  server.on("/image.jpg", HTTP_GET, [](AsyncWebServerRequest* request) {
    // Load the current image for the server on "/image.jpg"
    request->send(SPIFFS, imagePath[pos], "image/jpeg");
  });



  server.on("/D", HTTP_GET, [](AsyncWebServerRequest* request) {
    //If the User pressed on the delete button, the ESP deletes the current image
    //Deleted screen on display
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
    //Draw the next image instead of the deleted one
    tft.drawJpgFile(SPIFFS, imagePath[pos], 0,0);
  });
  
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
    // If the User pressed on the wifi button, the ESP sends him the form for the new SSID & Password
    request->send(SPIFFS, "/wifi.html", "text/html");
  });

  server.on("/formdata", HTTP_GET, [](AsyncWebServerRequest *request){
    // When the form is submitted the ESP gets the data and connects to the new network
    String SSID = request->getParam("SSID")->value();
    String Pass = request->getParam("Password")->value();
    
    Serial.print("SSID: ");
    Serial.println(SSID);
    
    Serial.print("Password: ");
    Serial.println(Pass);


    configura_wifi(SSID.c_str(), Pass.c_str());
    request->send(SPIFFS, "/index.html", "text/html");

  });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest* request) {
      //Image Upload Handler
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "Caricamento immagine completato");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    },
    [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {

      if (!index) {
        imgName[nUpload] = "/img" + String(nUpload) + ".jpg"; //name the image
        imgNameChar[nUpload] = new char[imgName[nUpload].length() + 1];
        strcpy(imgNameChar[nUpload], imgName[nUpload].c_str());
        Serial.println(imgNameChar[nUpload]);
        File file = SPIFFS.open(imgNameChar[nUpload], FILE_WRITE); //create its file in SPIFFS
        if (!file) {
          Serial.println("Errore nell'apertura del file");
          delete[] imgNameChar[nUpload];
          return;
        }
        file.write(data, len); //upload the image
        file.close();
      }
      else {
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
        //if the upload succeded, the ESP32 displays the image
        Serial.println("Caricamento immagine completato");
        tft.drawJpgFile(SPIFFS, imgNameChar[nUpload], 0, 0);
        imgN += 1;
        pos = imgN - 1;
        imagePath[pos] = imgNameChar[nUpload];
        nUpload += 1;
      }
    }
  );

  // Web Server initialization
  server.begin();
}

void loop() {
  //read the light sensor and modify the brightness of the screen accordingly
  int luce = analogRead(fr);
  analogWrite(5, map(luce, 0, 4095, 245,0));
  delay(100);
  
  //get the value of the X & Y axis of the joystick
  int x = map(analogRead(X_AXIS_PIN), 0, 4095, 0, 100);
  int y = map(analogRead(Y_AXIS_PIN), 0, 4095, 0, 100);

  
  if(millis()-t>500){ //debounce the joystick (check every 500ms to avoid multiple inputs)
    //if the x or y value reach 0 or 100, consider it an input and change the image
    if(x==100 && xdir!=1){
      xdir=1;
      t=millis();
      if (display)
        pos = (pos+1)%imgN;
      display = true;
      tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    }
    else if (x==0 && xdir!=-1)
    {
      xdir=-1;
      t=millis();
      if (display){
        pos -= 1;
        if(pos < 0)
          pos = imgN - 1;
      }
      display = true;
      tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    }
    else{
      xdir=0;
    }
    if(y==100 && ydir!=1){
      ydir=1;
      t=millis();
      if (display)
        pos = (pos+1)%imgN;  
      display = true;
      tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    }
    else if (y==0 && ydir!=-1)
    {
      ydir=-1;
      t=millis();
      if (display){
        pos -= 1;
        if(pos < 0)
          pos = imgN - 1;
      }
      display = true;
      tft.drawJpgFile(SPIFFS, imagePath[pos], 0, 0);
    }
    else{
      ydir=0;
    }
  }
}
