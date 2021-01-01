#include <SoftwareSerial.h>
#include <Wire.h>
#include <DHT.h>

#define DHT1PIN 2
#define DHT2PIN 3
#define DHTTYPE DHT11
DHT sensor1 (DHT1PIN,DHTTYPE);
DHT sensor2 (DHT2PIN,DHTTYPE);

String wireless = "XXXXXXXXXXXX";                    //Wifi Name
String wirelesspw = "***********";                         //Wifi Password
int rxPin = 10;
int txPin = 11;
String ip = "184.106.153.149";                             //ThingSpeak's IP

#define DEBUG true

SoftwareSerial esp(rxPin, txPin);

String kombi="3";                                          //For default plug condition

void setup() {  
  enbas:
  pinMode(7,OUTPUT);                                       //Set the relay as output
  digitalWrite(7,HIGH);                                    //Since I use an optocoupler relay in this project, I make the initial state of the relay low with high command. (Low level triggered relay)
  int hata0 = 0;                                           //Error counters for reset
  int hata1 = 0;
  int hata2 = 0;
  Serial.begin(9600);
  Serial.println("Başlatılıyor");
  esp.begin(115200);                                       //We will start the communication once with this command. After the first upload, this line will be overridden. 9600 will be valid.
  //esp.begin(9600); 
  esp.println("AT");                                       //Send AT command
  Serial.println("AT Gönderildi");
  while(!esp.find("OK"))
  {
    esp.println("AT");                                                       //The AT command is being sent again. 
    Serial.println("ESP8266 ile iletişim kurulamadı. Tekrar deneniyor"); 
    hata0=hata0+1;
    if(hata0>3)
    { 
      Serial.println("RESET!!!");

      goto enbas;                                                            //If the error count is exceeded, it is reset. program starts from the beginning
    }
  }
  esp.println("AT+UART_DEF=9600,8,1,0,0");                                   //We're reducing the speed from 115200 to 9600. After the first run, this line will be disabled like line "esp.begin(115200)"
  Serial.println("OK Komutu Alındı. İletişim kuruldu.");
  delay (1000);

  esp.println("AT+CWMODE=1");                                                //Set the mode of esp as 1.client mod
  Serial.println("Modül Ayarı Gönderildi");
  while(!esp.find("OK"))
  { 
    esp.println("AT+CWMODE=1");                                              //The client mode command is being sent again. 
    Serial.println("Modül Ayarı Yapılamadı. Tekrar deneniyor");
    hata1=hata1+1;
    if(hata1>3)
    { 
      Serial.println("RESET!!!");
      goto enbas;                                                            //If the error count is exceeded, it is reset. program starts from the beginning
    }
  }
  Serial.println("Modül Client olarak ayarlandı");
  delay (1000);
  Serial.println("Ağa Baglaniliyor...");
  esp.println("AT+CWJAP=\""+wireless+"\",\""+wirelesspw+"\"");               //Connection to the network
  while(!esp.find("OK"))                                                     //waiting for connection
  {
   delay(1);
   hata2=hata2+1;
   Serial.println("sure: ");
   Serial.println(hata2);
   if (hata2>10) 
   {
    Serial.println("RESET!!!");
    goto enbas;                                                              //Reset if the connection time is exceeded. the program starts from the beginning
   }
  }
  Serial.println("Ağa Bağlanıldı.");
  delay(1000);

}


void loop() 
{
  delay (1000);
  esp.println("AT+CIPSTART=\"TCP\",\""+ip+"\",80");                         //Connect to Thingspeak site via TCP
  if(esp.find("Error"))                                                     //Connection error check
  {
    Serial.println("AT+CIPSTART Error");
  }
  Serial.println("cekilen text: ");
  String readall = readdataonweb();                                         //The text returned from the function called readdataonweb is assigned to readall variable.
  int clearread=readall.indexOf('#');                                       //In the read message, we find the section we want from the general text with the help of # character to extract only the content of the page.
  readall=readall.substring(clearread+7,clearread+8);                       //I found the character I wanted according to the length of my text. You must set it according to your own text
  kombi = readall;
  Serial.print ("kombi deger: "); 
  Serial.println (kombi);             
  esp.println("AT+CIPCLOSE");                                               //Connection is terminating
  Serial.println("Baglantı Kapatildi.");
  delay(5000); 

  if (kombi == "0")                                                         //According to the value read, the relay (plug) becomes active or deactivated.
  {
    digitalWrite(7,HIGH);  
  }
  if (kombi == "1")
  {
    digitalWrite(7,LOW);  
  }
  
  float h1 = sensor1.readHumidity();                                         //Humidity value is read from the 1st sensor.
  float t1 =  (sensor1.readTemperature())-1;                                 //Temperature value is read from the 1st sensor. I have arranged the margin of error according to my own measurements. It may differ in your sensors.
  Serial.print("Sensor 1 Nem: ");
  Serial.println(h1);
  Serial.print("Sensor 1 Sicaklik: ");
  Serial.println(t1);
  delay (1000);

  float h2 = sensor2.readHumidity();                                         //Humidity value is read from the 2nd sensor.
  float t2 =  (sensor2.readTemperature())-3;                                 //Temperature value is read from the 2nd sensor. I have arranged the margin of error according to my own measurements. It may differ in your sensors.
  Serial.print("Sensor 2 Nem: ");
  Serial.println(h2);
  Serial.print("Sensor 2 Sicaklik: ");
  Serial.println(t2);
  delay (1000);
  
  esp.println("AT+CIPSTART=\"TCP\",\""+ip+"\",80");                           //We connect to Thingspeak site via TCP
  if(esp.find("Error"))                                                       //Connection error check 
    {
      Serial.println("AT+CIPSTART Error");
    }

  String yukle = "GET https://api.thingspeak.com/update?api_key=***********";     //You must type the write key of the channel you created on Thingspeak.
  yukle = yukle + "&field1=";
  yukle = yukle + String(kombi);
  yukle = yukle + "&field2=";
  yukle = yukle + String(t1);
  yukle = yukle + "&field3=";
  yukle = yukle + String(t2);
  yukle = yukle + "&field4=";
  yukle = yukle + String(h1);
  yukle = yukle + "&field5=";
  yukle = yukle + String(h2);
  yukle = yukle + "\r\n\r\n\r\n\r\n";
  esp.print("AT+CIPSEND=");                                   //Length of data to be sent to ESP.
  esp.println(yukle.length()+2);
  delay(2000);
  if(esp.find(">")){                                          //When the ESP8266 is ready, the commands in it are running.
    esp.print(yukle);                                         //Send the data.
    Serial.println(yukle);
    Serial.println("Veri gonderildi.");
    delay(1000);
  }
  Serial.println("Baglantı Kapatildi.");
  esp.println("AT+CIPCLOSE");                                 //Connection is terminating
  delay(300000);                                              //Delay 5 minute because data exchange is enough every 5 minutes in my project. It should be adjusted according to the project.


}



String readdataonweb()
{
  String rest = "AT+CIPSEND=90";
  rest =rest + "\r\n";
  sendData(rest, 3000, 0); 

  String hostt = "GET /apps/thinghttp/send_request?api_key=***********";         //You must type the read key you got from thinghttp app
  hostt = hostt + "\r\n";
  hostt = hostt +  "Host:api.thingspeak.com";
  hostt = hostt +  "\r\n\r\n\r\n\r\n\r\n";
  String text = sendData(hostt, 3000, 1);
  
  return (text);
}



String sendData(String komut, const int zamangecen, boolean debug)
{
  String response = "";
  esp.print(komut);
  long int Zaman = millis();
  while ( (Zaman + zamangecen) > millis())
  {
    while (esp.available())
    {
      char c = esp.read();
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
  }

  return response;
}
