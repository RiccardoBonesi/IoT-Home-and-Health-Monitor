#include <SPI.h>
#include <dht11.h>
#include "secrets.h"
#include <math.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

////////////////// CIRCUIT CONNECTION NOTES ///////////////////////
/*
   S va collegato al data
   A0 heartbeat
   D0 shock (cilindro nero) (VIBRATION SWITCH MODULE)
   D2 tilt (blu)
   D4 buzzer
   D5 button (KEY SWITCH MODULE)
   D7 temperature (dht11)
*/



////////////////////////////////////////////////// WIFI CONFIGURATION /////////////////////////////////////////////////////////////

char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password
int keyIndex = 0; // your network key Index number (needed only for WEP)

char username[] = USER;
char password[] = PASS;

IPAddress ip(149, 132, 182, 35);
IPAddress gateway(149, 132, 182, 1);
IPAddress dns(149, 132, 2, 3);
IPAddress subnet(255, 255, 255, 0);

int status = WL_IDLE_STATUS;
//WiFiServer server(80);

//-------- MQTT -----------
const char* mqtt_server = "149.132.182.184";
WiFiClient espClient;
PubSubClient client(espClient);

//////////////////////////////////////////////////////// SENSORS CONFIGURATION //////////////////////////////////////////////////////////////////////

// TEMPERATURE + HUMIDITY
#define DHT11PIN D7
dht11 dht11;

// TILT
#define tiltPin D2     // define the tilt switch sensor interfaces
int tilt_val ;         // define numeric variables val

// SHOCK
#define shockPin D0   // define the vibration sensor interface
int shock_val;        // define numeric variables val

// HEARTBEAT
#define samp_siz 4
#define rise_threshold 4
int heartbeatPin = 0;



// BUTTON
#define buttonPin D5        // define the key switch sensor interface
//int button_val ;          // define numeric variables val
int buttonState = 0;        // variable for reading the pushbutton status
int prevButtonState = 0;    // variable for reading the pushbutton status


// BUZZER
#define buzzerPin D8

/// Alarms and Thresholds
bool buttonPressed = false;
bool tilt_alarm = false;
bool shock_alarm = false;




//###########################################################################

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp ("eventTopic/monitor/start", topic) == 0) {
    heartbeatMonitoring();
  } 
  
  if (strcmp ("eventTopic/temperature/start", topic) == 0) {
    Serial.println("Temperature callback received");
    temperatureMonitoring();
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("outTopic","hello world");
      // ... and resubscribe
      // client.subscribe("assignment2/heartbeat/events");
      //      client.subscribe("arduino/led1/cmd");
      //      client.subscribe("arduino/led2/cmd");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}



void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.config(ip, gateway, subnet, dns);
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      delay(5000);
    }
    Serial.println("\nConnected to WiFi.");

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

  }

  client.subscribe("eventTopic/monitor/start");
    
  client.subscribe("eventTopic/temperature/start");  


  // TEMPERATURE + HUMIDITY
  pinMode(DHT11PIN, INPUT);

  // TILT
  pinMode (tiltPin, INPUT) ;

  // SHOCK
  pinMode (shockPin, INPUT) ;

  // BUTTON
  pinMode (buttonPin, INPUT);

  // BUZZER
  pinMode(buzzerPin, OUTPUT);
}

//###########################################################################



void loop() {

  //--------------- WIFI ---------------------------------
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.config(ip, gateway, subnet, dns);
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  client.subscribe("eventTopic/monitor/start");
  delay(50);  
  client.subscribe("eventTopic/temperature/start"); 



  //---------- TILT --------------
  tilt_val = digitalRead (tiltPin) ;// digital interface will be assigned a value of 3 to read val  
  if (tilt_val == HIGH) //When the tilt sensor detects a signal when the switch, LED flashes
  {
    Serial.println("NO TILT");
    tilt_alarm = false;
  }
  else
  {
    Serial.println("TILT");
    tilt_alarm = true;    
  }


  //---------- SHOCK ------------
  shock_val = digitalRead (shockPin) ; // read digital interface is assigned a value of 3 val  
  if (shock_val == HIGH) // NOTA: SHOCK MI FUNZIONA AL CONTRARIO
  {
    Serial.println("NO SHOCK");
    shock_alarm = false;
  }
  else
  {
    Serial.println("SHOCK");
    shock_alarm = true;
  }

  delay(500);


  //--------- BUTTON ---------
  buttonState = digitalRead(buttonPin);
  if (buttonState + prevButtonState == 1) {
    Serial.println("Button not pressed");
    Serial.println("");
    buttonPressed = false;

  }
  else {
    Serial.print("Button pressed");
    Serial.println("");
    prevButtonState = 0;
    buttonPressed = true;
  }

  //--------- HERTBEAT -------------
  /*
    static double oldValue = 0;
    static double oldChange = 0;

    int rawValue = analogRead (heartbitPin);
    double value = alpha * oldValue + (1 - alpha) * rawValue;

    //Serial.print (rawValue);
    //Serial.print (",");
    //Serial.println (value);
    oldValue = value;
    delay (100);
  */

  //heartbeatMonitoring();







  if (tilt_alarm) {
    client.publish("eventTopic", "tilt");
  }
  if (shock_alarm) {
    client.publish("eventTopic", "shock");
  }
  if (buttonPressed) {
    client.publish("eventTopic", "buttonPressed");
  }


  if (buttonPressed) {
    //digitalWrite (buzzerPin, HIGH);
    tone(buzzerPin, 200); // TODO da rivedere la frequenza perchè adesso se no sembro un mongoloide che gioca
  }
  else {
    //digitalWrite (buzzerPin, LOW);
    noTone(buzzerPin);
  } 


  delay(1000);
}






///////////////////////////////////////////////////

void temperatureMonitoring() {
  uint8_t chk = dht11.read(DHT11PIN);
  float temperature = readTemp();

  // uint32_t period = 50000;

  //client.publish("eventTopic/temperature/value", result);

  Serial.print("Sending temperature... value: ");
  Serial.println(temperature);

  char result[8]; // Buffer big enough for 7-character float
  dtostrf(temperature, 4, 6, result); // Leave room for too large numbers!

  client.publish("eventTopic/temperature/value", result);
  delay(250);

}


//------------

void heartbeatMonitoring() {
  Serial.println("Starting heartbeat monitoring...");
  float reads[samp_siz], sum;
  long int now, ptr;
  float last, reader, start;
  float first, second, third, before, print_value;
  bool rising;
  int rise_count;
  int n;
  long int last_beat;

  for (int i = 0; i < samp_siz; i++)
    reads[i] = 0;
  sum = 0;
  ptr = 0;

  uint32_t period = 50000;  //5 * 60000L;   5 minutes

  for (uint32_t tStart = millis();  (millis() - tStart) < period; )
  {
    // calculate an average of the sensor
    // during a 20 ms period (this will eliminate
    // the 50 Hz noise caused by electric light
    n = 0;
    start = millis();
    reader = 0.;
    do
    {
      reader += analogRead (heartbeatPin);
      n++;
      now = millis();
    }
    while (now < start + 20);
    reader /= n;  // we got an average

    // Add the newest measurement to an array
    // and subtract the oldest measurement from the array
    // to maintain a sum of last measurements
    sum -= reads[ptr];
    sum += reader;
    reads[ptr] = reader;
    last = sum / samp_siz;
    // now last holds the average of the values in the array

    // check for a rising curve (= a heart beat)
    if (last > before)
    {
      rise_count++;
      if (!rising && rise_count > rise_threshold)
      {
        // Ok, we have detected a rising curve, which implies a heartbeat.
        // Record the time since last beat, keep track of the two previous
        // times (first, second, third) to get a weighed average.
        // The rising flag prevents us from detecting the same rise more than once.
        rising = true;
        first = millis() - last_beat;
        last_beat = millis();

        // Calculate the weighed average of heartbeat rate
        // according to the three last beats
        print_value = 60000. / (0.4 * first + 0.3 * second + 0.3 * third);

        char result[8]; // Buffer big enough for 7-character float
        dtostrf(print_value, 4, 6, result); // Leave room for too large numbers!

        client.publish("eventTopic/monitor/values", result);
        delay(1000);

        Serial.print(print_value);
        Serial.print(" ..... ");
        Serial.print(result);
        Serial.print('\n');

        third = second;
        second = first;

      }
    }
    else
    {
      // Ok, the curve is falling
      rising = false;
      rise_count = 0;
    }
    before = last;


    ptr++;
    ptr %= samp_siz;

    yield();
  }

  Serial.println("Heartbeat monitoring finished.");

}




float readTemp() {
  float t = dht11.temperature;
  return t;
}

int readHum() {
  int h = dht11.humidity;
  return h;
}


void PrintTemp(float t) {
  Serial.print("temperature = ");
  Serial.println(t);
  if (t < -47 )
  {
    Serial.println("Failed to read from Temperature");
  }
  else
  {
    float temp_color = (t / 30) * 255;
  }
}
