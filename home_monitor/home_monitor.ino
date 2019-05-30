#include <SPI.h>
#include <WiFi101.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <dht11.h>
#include <Grove_LED_Bar.h>
//#include <MQTT.h>

#include "secrets.h"
#include <math.h>
#include "rgb_lcd.h"

//#include <MQTTClient.h>
#include <PubSubClient.h>

/////////////////////// CIRCUIT CONNECTION NOTES ////////////////
/*
  6 dht11
  A3 3 flame
  1 buzzer
  A0 sound
  A4 light
  0 led
*/

////////////////////////////////////////////////// MYSQL CONFIGURATION ///////////////////////////////////////////////////////////

byte mac_addr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server_addr(149, 132, 182, 128); // IP of the MySQL *server* here

char user[] = MYSQL_USER;              // MySQL user login username
char password[] = SECRET_MYSQL_PASS;   // MySQL user login password

// Sample query
char INSERT_DATA[] = "INSERT INTO test_arduino.hello_sensor (temperature, humidity, sound, flame, light, signal_quality, alarm, recorded) VALUES (%d,%d,%d,%d,%d,%d,%d, current_timestamp())";
char query[1024];

WiFiClient client;

MySQL_Connection conn((Client *)&client);

////////////////////////////////////////////////// WIFI CONFIGURATION /////////////////////////////////////////////////////////////

char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password
int keyIndex = 0; // your network key Index number (needed only for WEP)

IPAddress ip(149, 132, 182, 34);
IPAddress gateway(149, 132, 182, 1);
IPAddress dns(149, 132, 2, 3);
IPAddress subnet(255, 255, 255, 0);

int status = WL_IDLE_STATUS;
WiFiServer server(80);

///////////////////////////////////////////////// MQTT CONFIGURATION //////////////////////////////////////////////////////////////

const char* mqtt_server = "149.132.182.128";
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Function prototypes
void subscribeReceive(char* topic, byte* payload, unsigned int length);

//////////////////////////////////////////////////////// SENSORS CONFIGURATION //////////////////////////////////////////////////////////////////////

// TEMPERATURE + HUMIDITY
#define DHT11PIN 6
dht11 dht11;

// SOUND
const int pinAdc = A0;

// FLAME
int Led = 13 ;// define LED Interface
int buttonpin = 3; // define the flame sensor interface
int analoog = A3; // define the flame sensor interface

int val ;// define numeric variables val
float flame_value; //read analoog value

// LIGHT
//Grove_LED_Bar bar(3, 2, 0);  // Clock pin, Data pin, Orientation

// BUZZER
const int buzz = 1;

//LED
const int led = 0;

// LCD
const int colorR = 255;
const int colorG = 255;
const int colorB = 0;
rgb_lcd lcd;
String displayed_temp = "Not reading temperature sensor";
String app_name = "Home Monitoring";


//////////////////////////////////////

int temp_array[10] = {25};
int hum_array[10] = {27};
int sound_array[10] = {15};
int flame_array[10] = {900};
int light_array[10] = {700};
int rssi_array[10] = {0};
int array_length = 10;


bool temp_alarm_hot = false;
bool temp_alarm_cold = false;
bool hum_alarm = false;
bool sound_alarm = false;
bool flame_alarm = false;
bool light_alarm = false;
bool rssi_alarm = false;

bool alarm = false;

//###########################################################################

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  if (msg == "shock") {
    notifyShock();
  } else if (msg == "tilt") {
    notifyTilt();
  } else if (msg == "buttonPressed") {
    notifyButtonPressed();
  }
  Serial.println(msg);

}

void notifyTilt() {
  lcd.setRGB(255, 0, 0);
  lcd.setCursor(1, 1);
  lcd.print("Tilt detected");
  digitalWrite(0, HIGH);
  for (int i = 0; i < 3; i++) {
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    delay(500);
  }
  digitalWrite(0, LOW);
  LCD_default();
}

void notifyShock() {
  lcd.setRGB(255, 0, 0);
  lcd.setCursor(1, 1);
  lcd.print("Shock detected");
  digitalWrite(0, HIGH);
  for (int i = 0; i < 3; i++) {
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    delay(500);
  }
  digitalWrite(0, LOW);
  LCD_default();
}

void notifyButtonPressed() {
  lcd.setRGB(255, 0, 0);
  lcd.setCursor(1, 1);
  lcd.print("Assistance Required");
  digitalWrite(0, HIGH);
  for (int i = 0; i < 3; i++) {
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    delay(500);
  }
  digitalWrite(0, LOW);
  LCD_default();
}

void subscribeReceive(char* topic, byte* payload, unsigned int length)
{
  // Print the topic
  Serial.print("Topic: ");
  Serial.println(topic);

  // Print the message
  Serial.print("Message: ");
  for (int i = 0; i < length; i ++)
  {
    Serial.print(char(payload[i]));
  }

  // Print a newline
  Serial.println("");
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("arduino")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("outTopic","hello world");
      // ... and resubscribe
      // client.subscribe("assignment2/heartbeat/events");
      //      client.subscribe("arduino/led1/cmd");
      //      client.subscribe("arduino/led2/cmd");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(DHT11PIN, INPUT);
  pinMode(0, OUTPUT);
  Serial.begin(9600);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.config(ip, dns, gateway, subnet);
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      delay(5000);
    }
    Serial.println("\nConnected to WiFi.");

    if (conn.connect(server_addr, 3306, user, password)) {
      Serial.println("\nConnection to database success.");
      printWifiStatus();
      delay(1000);
    }
    else
      Serial.println("Connection to database failed.");
  }

  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);

  // LCD
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);
  lcd.setCursor(0, 0);
  lcd.print(app_name);

  pinMode(1, OUTPUT);
}

//###########################################################################



void loop() {

  /////////////// WIFI /////////////////////////////////////////
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.config(ip, dns, gateway, subnet);
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  if (!mqttClient.connected()) {
    reconnect();
    delay(100);
    mqttClient.subscribe("eventTopic");
  }
  delay(100);
  mqttClient.loop();
  delay(100);



  //mqttClient.publish("eventTopic", "sono mkr1000");
  //mqttClient.subscribe("eventTopic");


  Serial.println("Recording data.");

  // Temperature + Humidity
  uint8_t chk = dht11.read(DHT11PIN);
  int temperature = readTemp();
  int humidity = readHum();


  // Sound
  long sound_value = readSound();
  Serial.println("sound: " + (String) sound_value);
  delay(10);


  //////////// FLAME ////////////////////////////
  flame_value = analogRead(analoog);
  Serial.println("flame value: " + (String) flame_value);  // display tempature

  val = digitalRead (buttonpin) ;// digital interface will be assigned a value of 3 to read val
  if (val == HIGH) // When the flame sensor detects a signal, LED flashes
  {
    digitalWrite (Led, HIGH);
  }
  else
  {
    digitalWrite (Led, LOW);
  }


  // Light
  int light_value = readLight();


  long rssi = WiFi.RSSI();


  //////////////// THRESHOLD ////////////////////

  int i;
  for (i = 1; i < 10; i++) {
    temp_array[i] = temp_array[i - 1];
  }
  temp_array[0] = temperature;

  int sum = 0;

  for (int i = 0; i < 10; i++) {
    sum += temp_array[i];
  }
  int temp_avg = sum / 10;

  if (temp_avg > 35) {
    temp_alarm_hot = true;
  } else if (temp_avg < 0) {
    temp_alarm_cold = true;
  } else {
    temp_alarm_hot = false;
    temp_alarm_cold = false;
  }



  //////////

  for (i = 1; i < 10; i++) {
    hum_array[i] = hum_array[i - 1];
  }
  hum_array[0] = humidity;

  sum = 0;

  for (i = 0; i < 10; i++) {
    sum += hum_array[i];
  }
  int hum_avg = sum / 10;

  if (hum_avg > 50) {
    hum_alarm = true;
  } else {
    hum_alarm = false;
  }




  ////////

  for (i = 1; i < 10; i++) {
    sound_array[i] = sound_array[i - 1];
  }
  sound_array[0] = sound_value;

  sum = 0;

  for (i = 0; i < 10; i++) {
    sum += sound_array[i];
  }
  long sound_avg = sum / 10;

  if (sound_avg > 80) {
    sound_alarm = true;
  } else {
    sound_alarm = false;
  }




  /////////

  for (i = 1; i < 10; i++) {
    flame_array[i] = flame_array[i - 1];
  }
  flame_array[0] = flame_value;

  sum = 0;

  for (i = 0; i < 10; i++) {
    sum += flame_array[i];
  }
  int flame_avg = sum / 10;



  if (flame_avg < 300) {
    flame_alarm = true;
  } else {
    flame_alarm = false;
  }





  ///////////

  for (i = 1; i < 10; i++) {
    light_array[i] = light_array[i - 1];
  }
  light_array[0] = light_value;

  sum = 0;

  for (i = 0; i < 10; i++) {
    sum += light_array[i];
  }
  int light_avg = sum / 10;

  if (light_avg < 150) {
    light_alarm = true;
  } else {
    light_alarm = false;
  }




  //////////

  for (i = 1; i < 10; i++) {
    rssi_array[i] = rssi_array[i - 1];
  }
  rssi_array[0] = rssi;

  sum = 0;

  for (i = 0; i < 10; i++) {
    sum += rssi_array[i];
  }
  long rssi_avg = sum / 10;

  if (rssi_avg > 0) {
    rssi_alarm = true;
  } else {
    rssi_alarm = false;
  }








  //////

  if (flame_alarm) {
    digitalWrite(1, HIGH);
    lcd.setRGB(255, 0, 0);
    lcd.setCursor(1, 1);
    lcd.print("Flame detected");
  } else if (temp_alarm_hot) {
    digitalWrite(1, HIGH);
    lcd.setRGB(255, 0, 0);
    lcd.setCursor(1, 1);
    lcd.print("High temperature detected");
  } else if (temp_alarm_cold) {
    digitalWrite(1, HIGH);
    lcd.setRGB(255, 0, 0);
    lcd.setCursor(1, 1);
    lcd.print("Low temperature detected");
  }  else if (hum_alarm) {
    digitalWrite(1, HIGH);
    lcd.setRGB(255, 0, 0);
    lcd.setCursor(1, 1);
    lcd.print("High humidity detected");
  } else if (light_alarm) {
    lcd.setRGB(255, 0, 0);
    lcd.setCursor(1, 1);
    lcd.print("Low light");
  } else if (sound_alarm) {
    lcd.setRGB(255, 0, 0);
    lcd.setCursor(1, 1);
    lcd.print("High sound detected");
  } else if (rssi_alarm) {
    lcd.setRGB(255, 0, 0);
    lcd.setCursor(1, 1);
    lcd.print("Low wifi signal");
  } else {
    digitalWrite(1, LOW);
    LCD_default();
  }




  if (flame_alarm || temp_alarm_hot || temp_alarm_cold || hum_alarm || sound_alarm || light_alarm || rssi_alarm) {
    alarm = true;
  }
  else {
    alarm = false;
  }

  WriteMultiToDB(temperature, humidity, sound_value, (int) flame_value, light_value, rssi, alarm);

  delay(500);
}




int WriteMultiToDB(int temperature, int humidity, long sound_value, int flame_value, int light_value, long rssi, bool alarm) {
  // Initiate the query class instance
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);

  sprintf(query, INSERT_DATA, temperature, humidity, sound_value, flame_value, light_value, rssi, alarm);
  Serial.println(query);

  // Execute the query
  cur_mem->execute(query);
  // Note: since there are no results, we do not need to read any data
  // Deleting the cursor also frees up memory used
  delete cur_mem;
}

int readTemp() {
  int t = dht11.temperature;
  return t;
}

int readHum() {
  int h = dht11.humidity;
  return h;
}

long readSound() {
  long sound_value = analogRead(A0);
  return sound_value;
}

int readLight() {
  int light_value = analogRead(A4);
  return light_value;
}

void PrintTemp(float t) {
  // Serial.print("temperature = ");
  // Serial.println(t);
  if (t < -47 )
  {
    Serial.println("Failed to read from Temperature");
    lcd.clear();
    lcd.setRGB(128, 10, 10);
    displayed_temp = "Sensor not connected";
    //lcd.print(displayed_temp);


  }
  else
  {
    float temp_color = (t / 30) * 255;
    lcd.clear();
    lcd.setRGB(200, 200, (int) temp_color);
    displayed_temp = String(t) + " C";


  }
  lcd.setCursor(0, 0);
  lcd.print(app_name);

  lcd.setCursor(0, 1);
  lcd.print(displayed_temp);
}




void LCD_default() {
  String system_status = "Standby mode";

  if (WiFi.status() == WL_CONNECTED) {
    system_status = getWiFiStatus();
  }
  lcd.clear();

  lcd.setRGB(colorR, colorG, colorB);
  lcd.setCursor(0, 0);
  lcd.print(app_name);
  lcd.setCursor(1, 1);
  lcd.print("                ");

}

String getWiFiStatus() {
  IPAddress ip = WiFi.localIP();
  String ipaddress = String(ip[0]) + String(".") + \
                     String(ip[1]) + String(".") + \
                     String(ip[2]) + String(".") + \
                     String(ip[3]) ;

  //String netInfo = WiFi.SSID();
  //netInfo = netInfo + " IP " + ipaddress;
  String netInfo = " IP " + ipaddress;
  return netInfo;


}

void printWifiStatus() {

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
