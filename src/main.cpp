#include <Arduino.h>
#include "Colors.h"
#include "IoTicosSplitter.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

String dId = "121212";
String webhook_pass = WEBHOOK_PASS;
String webhook_endpoint = WEBHOOK_ENDPOINT;
const char* mqtt_server = MQTT_SERVER;

// WiFi Credentials
const char *wifi_ssid = WIFI_SSID;
const char *wifi_password = WIFI_PASS;

//PINS
#define led 2


//Global Vars
WiFiClient espclient;
PubSubClient client(espclient);
long lastReconnectAttemp = 0;
int prev_ph = 0;
int prev_ppm = 0;
DynamicJsonDocument mqtt_data_doc(2048);

//PH varaibles
float calibration_value = 23.51 - 0.7; //21.34 - 0.7
int phval = 0; 
unsigned long int avgval; 
int buffer_arr[10],temp;
float ph_act;

//TDS variables
#define TdsSensorPin 34
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point

int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 15;       // current temperature for compensation

//Functions definitions
bool get_mqtt_credentials();
void check_mqtt_connection();
void send_data_to_broker();
bool reconnect();
void process_sensors();
void clear();
float get_ph();
int getMedianNum(int bArray[], int iFilterLen);
void get_ppm();


void setup()
{

  Serial.begin(921600);
  pinMode(led, OUTPUT);
  pinMode(TdsSensorPin,INPUT);
  clear();

  Serial.print(underlinePurple + "\n\n\nWiFi Connection in Progress" + fontReset + Purple);

  WiFi.begin(wifi_ssid, wifi_password);

  int counter = 0;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;

    if (counter > 10)
    {
      Serial.print("  ⤵" + fontReset);
      Serial.print(Red + "\n\n         Ups WiFi Connection Failed :( ");
      Serial.println(" -> Restarting..." + fontReset);
      delay(2000);
      ESP.restart();
    }
  }

  Serial.print("  ⤵" + fontReset);

  //Printing local ip
  Serial.println(boldGreen + "\n\n         WiFi Connection -> SUCCESS :)" + fontReset);
  Serial.print("\n         Local IP -> ");
  Serial.print(boldBlue);
  Serial.print(WiFi.localIP());
  Serial.println(fontReset);
}

void loop()
{
  check_mqtt_connection();
  send_data_to_broker();
  //get_ppm();
  process_sensors();
  serializeJsonPretty(mqtt_data_doc, Serial);
}


void process_sensors(){


  //get ph simulation
  int ph = random(3, 10);
 // float ph = get_ph();
  mqtt_data_doc["variables"][0]["last"]["value"] = ph;
  mqtt_data_doc["variables"][0]["last"]["save"] = 1;


  //save ph?
  /*int dif = ph - prev_ph;
  if (dif < 0) {dif *= -1;}

  if (dif >= 0.9) {
    mqtt_data_doc["variables"][0]["last"]["save"] = 1;
  }else{
    mqtt_data_doc["variables"][0]["last"]["save"] = 0;
  }

  prev_ph = ph;*/

  //get ppm simulation
  //int ppm = random(1, 500);
  get_ppm();
  float ppm = tdsValue;
  /*Serial.println("PPM normal:");
  Serial.print(ppm);
  Serial.println("PPM acortado:");
  Serial.println(ppm,0);*/
  mqtt_data_doc["variables"][1]["last"]["value"] = ppm;
  mqtt_data_doc["variables"][1]["last"]["save"] = 1;

  //save ppm?
  /*dif = ppm - prev_ppm;
  if (dif < 0)
  {
    dif *= -1;
  }

  if (dif >= 20)
  {
    mqtt_data_doc["variables"][1]["last"]["save"] = 1;
  }
  else
  {
    mqtt_data_doc["variables"][1]["last"]["save"] = 0;
  }

  prev_ppm = ppm;*/

}


//TEMPLATE

long varsLastSend[6];

void send_data_to_broker(){
  long now = millis();

  for(int i =0; i < mqtt_data_doc["variables"].size(); i++){
    if(mqtt_data_doc["variables"][i]["variableType"] == "output"){
      continue;
    }

    int freq= mqtt_data_doc["variables"][i]["variableSendFreq"];

    if(now - varsLastSend[i] > freq *1000){
      varsLastSend[i] = millis();

      String str_root_topic = mqtt_data_doc["topic"];
      String str_variable = mqtt_data_doc["variables"][i]["variable"];
      String topic = str_root_topic + str_variable + "/sdata";

      String toSend = "";

      serializeJson(mqtt_data_doc["variables"][i]["last"], toSend);
      client.publish(topic.c_str(), toSend.c_str());
    }
  }
}

bool reconnect()
{

  if (!get_mqtt_credentials())
  {
    Serial.println(boldRed + "\n\n      Error getting mqtt credentials :( \n\n RESTARTING IN 10 SECONDS");
    Serial.println(fontReset);
    delay(10000);
    ESP.restart();
  }

  //Setting up Mqtt Server
  client.setServer(mqtt_server, 1883);

  Serial.print(underlinePurple + "\n\n\nTrying MQTT Connection" + fontReset + Purple + "  ⤵");

  String str_client_id = "device_" + dId + "_" + random(1, 9999);
  const char *username = mqtt_data_doc["username"];
  const char *password = mqtt_data_doc["password"];
  String str_topic = mqtt_data_doc["topic"];

  if (client.connect(str_client_id.c_str(), username, password))
  {
    Serial.print(boldGreen + "\n\n         Mqtt Client Connected :) " + fontReset);
    delay(2000);

    client.subscribe((str_topic + "+/actdata").c_str());
    delay(2000);
    return true;
  }
  else
  {
    Serial.print(boldRed + "\n\n         Mqtt Client Connection Failed :( " + fontReset);
  }
}

void check_mqtt_connection()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(Red + "\n\n         Ups WiFi Connection Failed :( ");
    Serial.println(" -> Restarting..." + fontReset);
    delay(15000);
    ESP.restart();
  }

  if (!client.connected())
  {

    long now = millis();

    if (now - lastReconnectAttemp > 5000)
    {
      lastReconnectAttemp = millis();
 
      bool rec = reconnect();
      if (rec)
      {
        lastReconnectAttemp = 0;
      }
    }
  }
  else
  {
    client.loop();
  }
}

bool get_mqtt_credentials()
{

  Serial.print(underlinePurple + "\n\n\nGetting MQTT Credentials from WebHook" + fontReset + Purple + "  ⤵");
  delay(1000);

  String toSend = "dId=" + dId + "&password=" + webhook_pass;

  HTTPClient http;
  http.begin(webhook_endpoint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int response_code = http.POST(toSend);

  if (response_code < 0)
  {
    Serial.print(boldRed + "\n\n         Error Sending Post Request :( " + fontReset);
    http.end();
    return false;
  }

  if (response_code != 200)
  {
    Serial.print(boldRed + "\n\n         Error in response :(   e-> " + fontReset + " " + response_code);
    http.end();
    return false;
  }

  if (response_code == 200)
  {
    String responseBody = http.getString();

    Serial.print(boldGreen + "\n\n         Mqtt Credentials Obtained Successfully :) " + fontReset);

    deserializeJson(mqtt_data_doc, responseBody);
    http.end();
    delay(1000);
  }

  return true;
}

void clear()
{
  Serial.write(27);    // ESC command
  Serial.print("[2J"); // clear screen command
  Serial.write(27);
  Serial.print("[H"); // cursor to home command
}


//PH method
float get_ph(){
  for(int i=0;i<10;i++) 
 { 
  buffer_arr[i]=analogRead(35);
  delay(30);
 }
  for(int i=0;i<9;i++)
  {
    for(int j=i+1;j<10;j++)
  {
    if(buffer_arr[i]>buffer_arr[j])
    {
      temp=buffer_arr[i];
      buffer_arr[i]=buffer_arr[j];
      buffer_arr[j]=temp;
    }
  }
  }
  avgval=0;
  for(int i=2;i<8;i++)
  avgval+=buffer_arr[i];
  float volt=(float)avgval*3.3/4096.0/6;  
  //Serial.print("Voltage: ");
  //Serial.println(volt);
  ph_act = -5.70 * volt + calibration_value;

  //Serial.print("pH Val: ");
  //Serial.println(ph_act);
  return ph_act;
}

// median filtering algorithm PPM
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

void get_ppm(){
  static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      /*Serial.println("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.print("ppm");*/
      //return tdsValue;
    }
  }
}