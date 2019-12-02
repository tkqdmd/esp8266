#include <ESP8266WiFi.h>
#include "Gsender.h"
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <WiFiUdp.h>
 
Adafruit_BMP085 bmp;
//DHT config
#define DHTPIN 12     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);

// Host
String apiKey = "L58DKBT8WS7PNQLD";
const char* server = "api.thingspeak.com";

#pragma region Globals
uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
#pragma endregion Globals
WiFiUDP Udp;

void setup()
{
    Serial.begin(115200);
    // Init DHT
    dht.begin();
    //Wire.begin (4, 5);
    if (!bmp.begin()) 
    {
      Serial.println("Could not find BMP180 or BMP085 sensor at 0x77");
      while (1) {}
    }
    //SmartConfig wifi
    int cnt = 0;
    
    WiFi.mode(WIFI_STA);
  //Chờ kết nối
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(cnt++ >= 10){
       WiFi.beginSmartConfig();
       while(1){
           delay(1000);
           //Kiểm tra kết nối thành công in thông báo
           if(WiFi.smartConfigDone()){
             Serial.println("SmartConfig Success");
             break;
           }
        }
     }
  
    Serial.println("");
    Serial.println("");
    
    WiFi.printDiag(Serial);
    // Khởi tạo server
    Udp.begin(49999);
    Serial.println("Server started");
  
    // In địa chỉ IP
    Serial.println(WiFi.localIP());
  }
}

void loop(){
    // Nhận gói tin gửi từ ESPTouch
    Udp.parsePacket();
    //In IP của ESP8266
    while(Udp.available()){
      Serial.println(Udp.remoteIP());
      Udp.flush();
      delay(5);
    }
    
   
    int error = 0;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(t) || isnan(h)) {
        Serial.println("Failed to read from DHT sensor!");
        error = 1;
    }else{
        Serial.print("Humidity: ");
        Serial.print(String(h, 1).c_str());
        Serial.print(" %\t");
        Serial.print("Temperature: ");
        Serial.print(String(t, 1).c_str());
        Serial.println(" *C ");
     }
    float tempbmp180 = bmp.readTemperature();
    float p = bmp.readPressure();
    if (isnan(tempbmp180) || isnan(p) || tempbmp180 > 200) {
        Serial.println("Failed to read from BMP sensor!");
        error = error + 2;
    }
    Serial.println("BMP180");
    Serial.print("Temperature = ");
    Serial.print(bmp.readTemperature());
    Serial.println(" Celsius");
   
    Serial.print("Pressure = ");
    Serial.print(bmp.readPressure());
    Serial.println(" Pascal");
    Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
    String warningMail = "thaptam0@gmail.com";
    if(error == 1){
           String subject = "DHT22 Warning!";
           String warning = String("Failed to read from DHT22 sensor!");
           if(gsender->Subject(subject)->Send(warningMail, warning)) {
              Serial.println("Message send.");
            } else {
              Serial.print("Error sending message: ");
              Serial.println(gsender->getError());
            }
            delay(120000);
            return;
        }else if(error == 2){
           String subject = "BMP180-GY68 Warning!";
           String warning = String("Failed to read from BMP180 & GY68 sensor!");
           if(gsender->Subject(subject)->Send(warningMail, warning)) {
              Serial.println("Message send.");
            } else {
              Serial.print("Error sending message: ");
              Serial.println(gsender->getError());
            } 
            delay(120000);
            return;
        } else if(error == 3){
            String subject = "DHT22 & BMP180-GY68 Warning!";
             String warning = String("Failed to read from DHT22 & BMP180-GY68 sensors!");
             if(gsender->Subject(subject)->Send(warningMail, warning)) {
                Serial.println("Message send.");
              } else {
                Serial.print("Error sending message: ");
                Serial.println(gsender->getError());
              } 
              delay(120000);
              return;
        }
    
    //Check temperature
        if(t>30){
           String subject = "High-Temperature Warning!";
           String tempWarning = String("The temperature of your room is so high!")
           + String("         Temperature:") + String(t, 1).c_str()+String(" C,")
           + String("  Humidity:") + String(h,1).c_str()+String(" %,")
           + String("  Pressure:") + String(p,1).c_str()+String(" Pascal."); 
           //Send mail
            if(gsender->Subject(subject)->Send(warningMail, tempWarning)) {
              Serial.println("Message send.");
            } else {
              Serial.print("Error sending message: ");
              Serial.println(gsender->getError());
            }
        }
        else if(t<10){
           String subject = "Low-Temperature Warning!";
           String tempWarning = String("The temperature of your room is so low!")
           + String("%\nTemperature:%\t") + String(t, 1).c_str()
           + String("%\nHumidity:%\t") + String(h,1).c_str()
           + String("%\nPressure:%\t") + String(p,1).c_str();
           if(gsender->Subject(subject)->Send(warningMail, tempWarning)) {
              Serial.println("Message send.");
            } else {
              Serial.print("Error sending message: ");
              Serial.println(gsender->getError());
            } 
        }
        

        
    Serial.print("Connecting to ");
    Serial.println(server);
    
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(server, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    String url = "/update?key=";
    url += apiKey;
    url += "&field1=";
    url += String(t);
    url += "&field2=";
    url += String(h);
    url += "&field3=";
    url += String(tempbmp180);
    url += "&field4=";
    url += String(p);
    url += "\r\n";
 // Request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + server + "\r\n" +
               "Connection: close\r\n\r\n");
    delay(1000);
 
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
  
    Serial.println();
    Serial.println("closing connection");
  
    // Repeat every 30 seconds
    delay(60000);
}
