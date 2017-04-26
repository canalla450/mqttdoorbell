#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Configure ADC so ESP.getVcc() can be used
ADC_MODE(ADC_VCC);

// Debugging
#define DEBUG 1 // Set to 1 to output to serial
#define PRINT_DEBUG(STR) { if (DEBUG) Serial.println(STR); }

// Configure Network
const char* ssid = "WiFi-SSID";     // WiFi SSID
const char* password = "pa55w0rd";  // WiFi password
IPAddress ip(192, 168, 1, 2);       // IP Address - comma separated
IPAddress gateway(192, 168, 1, 1);  // Gateway Address - comma separated
IPAddress subnet(255, 255, 255, 0);   // Subnet Mask - comma separated
IPAddress dns(192, 168, 1, 1);      // DNS server - comma separated

// Configure MQTT
const char* mqtt_server = "192.168.1.34";               // MQTT server ip address
const char* voltagetopic = "ESP8266/doorbell/voltage";  // Topic to send voltage information
const char* statustopic = "ESP8266/doorbell/status";    // Topic to send button press status

WiFiClient espClient;
PubSubClient client(espClient);

// Connect to WiFi
void connectWifi(const char* ssid, const char* password) {
  int timeout;

  if (WiFi.status() != WL_CONNECTED) {
    PRINT_DEBUG("Connecting to " + String(ssid));
    WiFi.config(ip, gateway, subnet, dns);
    WiFi.begin(ssid, password);
    for (timeout = 0; (WiFi.status() != WL_CONNECTED) && (timeout < 1000); timeout ++) {
      delay(10);
    }
    if (WiFi.status() != WL_CONNECTED) {
      PRINT_DEBUG("WiFi connection failed");
    } else {
      PRINT_DEBUG("WiFi connected");
    }
  } else PRINT_DEBUG("WiFi already connected");

  PRINT_DEBUG("IP address: " + String(WiFi.localIP()));
}

// Flash the LED
void flashLED(float frequency, int duration, float duty){
  unsigned int off_interval = (1000.0 / frequency) * (1.0 - duty);
  unsigned int on_interval = (1000.0 / frequency) * (duty);

  while (duration > 0) {
    // LED off
    digitalWrite(0, HIGH);
    delay(off_interval);
    duration -= off_interval;
    
    if (duration > 0) {
      // LED on
      digitalWrite(0, LOW);
      delay(on_interval);
      duration -= on_interval;
    }
  }
  
  // Always Switch Off
  digitalWrite(0, HIGH);
}

void setup() {
  if (DEBUG) Serial.begin(115200);
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH); // LED off
  PRINT_DEBUG("Reset reason: " + ESP.getResetReason());
  connectWifi(ssid, password);
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) digitalWrite(0, LOW); // LED on

  char volt[5];
  double voltage = ESP.getVcc() / 1000.0;
  dtostrf(voltage,5,4, volt);
  
  PRINT_DEBUG("Vcc: " + String(voltage) + "v");

  client.connect("ESP8266Client");
 
  bool v = client.publish(voltagetopic, volt);
  bool s = client.publish(statustopic, "ON");

  if (v && s) flashLED(1, 1000, 0.8);

  PRINT_DEBUG("Sleeping");
  pinMode(0, INPUT_PULLUP); // GPIO 0 back to high impedance mode
  //digitalWrite(0, HIGH); // LED off
  ESP.deepSleep(0, RF_CAL);
}
