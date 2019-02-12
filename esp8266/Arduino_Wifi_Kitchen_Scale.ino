#include <HX711.h>

// it might crash with wdt ... go fix the hx711.cpp - there's a yield overwrite that breaks it!

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

HX711 scale;
// Update these with values suitable for your network.

const char* ssid = "wifissid";
const char* password = "wifipassword";
const char* mqtt_server = "mqttbroker";
const char* inTopic = "kitchen_scale/cmnd";
const char* cmndOutTopic = "kitchen_scale/state";
const char* calibrationcmndOutTopic = "kitchen_scale/calibration";
const char* outTopic = "kitchen_scale/out";

// scale parameters
const int ReadInterval = 25; // milliseconds
const int ReadTimes = 3; // how many values should be averaged per Read Interval
float calibration_factor = 336.5;
float measurevalue = 0.0;
int buttonState = LOW;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
char commandinput[50];
int value = 0;

void setup() {

  setup_scale();
  
  Serial.begin(115200);
  setup_wifi();
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(0, INPUT);
  //pinMode(0, OUTPUT);
  //digitalWrite(0, LOW);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_scale() {
Serial.println("Initializing the scale");
  // parameter "gain" is ommited; the default value 128 is used by the library
  // HX711.DOUT  - pin #A1
  // HX711.PD_SCK - pin #A0
  scale.begin(12, 14);
  delay(1);
  scale.set_scale();
  delay(1);
  scale.tare();  //Reset the scale to 0
  delay(1);
  scale.set_scale(calibration_factor);
  scale.tare();               // reset the scale to 0
  delay(1);
  long zero_factor = scale.read_average(5);
  delay(1);
  scale.tare();                // reset the scale to 0
 
  Serial.println("Set-up done.");
}

void setup_wifi() {

  //ESP.eraseConfig();
  WiFi.persistent(false);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  //snprintf(commandinput,75,"%s",payload);
  
  
  strncpy(commandinput, (char*)payload, length);
  commandinput[length]=0;
  Serial.println(commandinput);
  // Switch on the LED if an 1 was received as first character
  if(strcmp(commandinput, "tare") == 0) {
    Serial.println("TARE");
    scale.tare();
    client.publish(cmndOutTopic, "tare done.");
  } else {
    if(strcmp(commandinput, "cal+") == 0) {
      Serial.println("Calibrationfaktor+");
      calibration_factor = calibration_factor+0.1;
      scale.set_scale(calibration_factor);
      scale.tare();               // reset the scale to 0
      dtostrf(calibration_factor, 2, 1, msg);
      //snprintf(msg,75,"%f",(int)calibration_factor);
      client.publish(calibrationcmndOutTopic, msg);
    }
    else
    if(strcmp(commandinput, "cal-") == 0) {
      Serial.println("Calibrationfaktor-");
      calibration_factor = calibration_factor-0.1;
      scale.set_scale(calibration_factor);
      scale.tare();               // reset the scale to 0
      dtostrf(calibration_factor, 2, 1, msg);
      //snprintf(msg,75,"%f",(int)calibration_factor);
      client.publish(calibrationcmndOutTopic, msg);
    }
    else
    if(strcmp(commandinput, "cal--") == 0) {
      Serial.println("Calibrationfaktor--");
      calibration_factor = calibration_factor-1.0;
      scale.set_scale(calibration_factor);
      scale.tare();               // reset the scale to 0
      dtostrf(calibration_factor, 2, 1, msg);
      //snprintf(msg,75,"%f",(int)calibration_factor);
      client.publish(calibrationcmndOutTopic, msg);
    }    
    else
    if(strcmp(commandinput, "cal++") == 0) {
      Serial.println("Calibrationfaktor++");
      calibration_factor = calibration_factor+1.0;
      scale.set_scale(calibration_factor);
      scale.tare();               // reset the scale to 0
      dtostrf(calibration_factor, 2, 1, msg);
      //snprintf(msg,75,"%f",(int)calibration_factor);
      client.publish(calibrationcmndOutTopic, msg);
    }    
    else
      Serial.println("OTHER");
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266ClientKitchenScale2")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish(outTopic, "hello world");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  yield();
  Serial.print("Value: \t");
  measurevalue = scale.get_units(ReadTimes);
  Serial.print(measurevalue, 2);
  Serial.println("g");
  //snprintf (msg, 75, "%.2f",measurevalue);
  //dtostrf(measurevalue, 1, 1, msg);
  snprintf(msg,75,"%d",(int)measurevalue);
  Serial.println(msg);
  client.publish(outTopic, msg);
  scale.power_down();              // put the ADC in sleep mode
  delay(ReadInterval);
  scale.power_up();
  
  buttonState = digitalRead(0); //GPIO 0
      // check if the pushbutton is pressed.
      // if it is, the buttonState is LOW:
  if (buttonState == LOW) {
     Serial.println("Tare");
     delay(250);
     scale.tare();
     //delay(100);
  }
  /*long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(outTopic, msg);
  }*/
}
