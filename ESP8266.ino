 #include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>


StaticJsonBuffer<256> jb;
JsonObject& obj = jb.createObject();

// Define NTP Client to get time
WiFiUDP u;
NTPClient n(u, "1.asia.pool.ntp.org");

WiFiClient client;
// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;
long save;
String combine1, combine2;
long last = 0; 

String apiKey = "CCG2HZG6PPSJOXNO";
const char* server = "api.thingspeak.com";

//#define FIREBASE_HOST "test-3bc03-default-rtdb.firebaseio.com"
//#define FIREBASE_AUTH "KFgSM9VKDs8rpukEpMApHwUaLren1YrSNdjaPpBY"
//#define FIREBASE_HOST "date-d8a3d-default-rtdb.firebaseio.com"    //Dunng
//#define FIREBASE_AUTH "QVmVwSI8FK5gZO1DanlbY5zm20WS0ivrxxFRsVco"   //Dung
//#define FIREBASE_HOST "datn-c02e3-default-rtdb.firebaseio.com"    //Dunng
//#define FIREBASE_AUTH "JhTvwrvgFef1x3SQUMIQ0Dy76V2l2PDzst1EWtia"   //Dung
#define FIREBASE_HOST "project-3340462344370439766-default-rtdb.firebaseio.com"    //Dunng
#define FIREBASE_AUTH "7Wtu1KKVdaK3WGVjWQBS5V3GJV9H9Rs8FkNK22od"   //Dung


#define WIFI_SSID "DuongLinh"
#define WIFI_PASSWORD "dao@12345"

unsigned long epochTime;
float state_C[4];
int timesave[3];
// int a=0, b=0, c=0, d=0, e=0, f=0;
String a,b,c,d,e,f;
int g = 1;


void UART();
void sendToBase();
void gettime();
void combineval();
void warningNH3_Pork();
void warningH2S_Pork();
void warningNH3_Fish();
void warningH2S_Fish();
void warningNH3_Chic();
void warningH2S_Chic();
void sendToBase();

void setup() {
  Serial.begin(9600);

  // ================================== WIFI CONNECTION =======================================//
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  // ===========================================================================================//
  
  // Initialize a NTPClient to get time
  n.begin();
 
//  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
 Firebase.begin("test-3bc03-default-rtdb.firebaseio.com","KFgSM9VKDs8rpukEpMApHwUaLren1YrSNdjaPpBY");
  last = millis();

}

void loop(){
  UART();
  gettime();
  combineval();
  warning_Pork();
//  warningH2S_Pork();
  warning_Fish();
//  warningH2S_Fish();
  warning_Chic();
//  warningH2S_Chic();
//  if(millis() - last >= 60000){
//    last = millis();
//    sendToBase();
//  }
  sendToBase();
  if(millis()- last >= 300000){
       last = millis();
       sendToBase5p();
  }
}


void UART(){
    if(Serial.available()>0){
//   value = Serial.write(Serial.read());
    String val0, val1, val2, val3;

    Serial.print("Temp: ");
    val0 = Serial.readStringUntil('!');
    state_C[0] = val0.toInt();
    if(state_C[0] < 20){
      state_C[0] = 30;
    }
    Serial.print(state_C[0]);
  
    Serial.print("\n"); 
    Serial.print("Humid: ");
    val1 = Serial.readStringUntil('@');
    state_C[1] = val1.toInt();
    Serial.print(state_C[1]);

    Serial.print("\n");
    val2 = Serial.readStringUntil('#');
    state_C[2] = val2.toFloat();
    Serial.print("nh3: ");
    Serial.print(state_C[2]);
    
    Serial.print("\n");
    Serial.print("h2s: ");
    val3 = Serial.readStringUntil('$');
    state_C[3] = val3.toFloat();
    Serial.print(state_C[3]);
    
    
    Serial.print("\n"); 
    Serial.print("\n"); 

//    delay(1000);
  }
}

void gettime(){

  n.update();
    Serial.print(n.getHours());
    Serial.print(":");
    Serial.print(n.getMinutes());
    Serial.print(":");
    Serial.print(n.getSeconds());
    Serial.print("\n");
//  Serial.println(n.getFormattedTime());
    formattedDate = n.getFormattedDate();

    timesave[0] = n.getHours();
    timesave[1] = n.getMinutes();
    timesave[2] = n.getSeconds();
    save = timesave[0]*3600 + timesave[1]*60 + timesave[2];
      // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);

  epochTime = n.getEpochTime();
}
void combineval(){
  combine1 = String(state_C[2])  + " / " + String(epochTime) ;
  combine2 = String(state_C[3])  + " # " + String(epochTime) ;
}

void warning_Pork(){
  if(state_C[2] < 10 && state_C[3] < 0.13){
    a = "Pork is Good!";
  }
  else if((state_C[2] >= 10 && state_C[2] <= 23) || (state_C[3] >= 0.13 && state_C[3] <= 0.3)){
    a = "You should eat pork soon!";
  }
  else if(state_C[2] > 23 || state_C[3] > 0.3){
    a = "Throw pork away!!!";
  }
}


//void warningH2S_Pork(){
//  if(state_C[3] < 0.13){
//    b = "Pork is Good!";
//  }
//  else if(state_C[3] >= 0.13 && state_C[3] <= 0.3){
//    b = "You should eat pork soon!";
//  }
//  else if(state_C[3] > 0.3){
//    b = "Throw pork away!!!";
//  }
//}

void warning_Fish(){
  if(state_C[2] < 10 && state_C[3] < 0.13){
    c = "Fish is Good!";
  }
  else if((state_C[2] >= 10 && state_C[2] <= 23) || state_C[3] >= 0.13 && state_C[3] <= 0.3){
    c = "You should eat fish soon!";
  }
  else if(state_C[2] > 23 || state_C[3] > 0.3){
    c = "Throw fish away!!!";
  }
}


//void warningH2S_Fish(){
//  if(state_C[3] < 0.13){
//    d = "Fish is Good!";
//  }
//  else if(state_C[3] >= 0.13 && state_C[3] <= 0.3){
//    d = "You should eat fish soon!";
//  }
//  else if(state_C[3] > 0.3){
//    d = "Throw fish away!!!";
//  }
//}

void warning_Chic(){
  if(state_C[2] < 10 && state_C[3] < 0.13){
    e = "Chicken is Good!";
  }
  else if((state_C[2] >= 10 && state_C[2] <= 23) || (state_C[3] >= 0.13 && state_C[3] <= 0.3)) {
    e = "You should eat chicken soon!";
  }
  else if(state_C[2] > 23 || state_C[3] > 0.3){
    e = "Throw chicken away!!!";
  }
}


//void warningH2S_Chic(){
//  if(state_C[3] < 0.13){
//    f = "Chicken is Good!";
//  }
//  else if(state_C[3] >= 0.13 && state_C[3] <= 0.3){
//    f = "You should eat chicken soon!";
//  }
//  else if(state_C[3] > 0.3){
//    f = "Throw chicken away!!!";
//  }
//}

void sendToBase(){
  obj["NH3RT"] = state_C[2];
  obj["H2SRT"] = state_C[3];
  obj["TemperatureRT"] = state_C[0];  
  obj["HumidityRT"] = state_C[1]; 
//  obj["NH3_S_Pork"] = a;
//  obj["H2S_S_Pork"] = b; 
//  obj["NH3_S_Fish"] = c;
//  obj["H2S_S_Fish"] = d; 
//  obj["NH3_S_Chic"] = e;
//  obj["H2S_S_Chic"] = f; 
  Firebase.set("/JSON/", obj);

//  Firebase.setString("/Signal_Pork",a );
//  Firebase.setString("/Signal_Fish",c );
//  Firebase.setString("/Signal_Chic",e );
//  Firebase.setString("/H2S_S_Fish",d );
//  Firebase.setString("/NH3_S_Chic",e );
//  Firebase.setString("/H2S_S_Chic",f );

}



void sendToBase5p(){
//  obj["NH3RT"] = state_C[2];
//  obj["H2SRT"] = state_C[3];
//  obj["TemperatureRT"] = state_C[0];  
//  obj["HumidityRT"] = state_C[1]; 
//  obj["NH3_Signal"] = a;
//  obj["H2S_Signal"] = b; 
//  Firebase.set("/JSON/", obj);

//  Firebase.pushString("/NH3",String(state_C[2]));
//  Firebase.pushString("/H2S",String(state_C[3]));

//  Firebase.pushString("/Temperature",String(state_C[0]));
//  Firebase.pushString("/Humidity",String(state_C[1]));

  Firebase.pushString("/NH3",String(combine1));
  Firebase.pushString("/H2S",String(combine2));
  
  Firebase.pushString("/Date", String(dayStamp));
  
}
