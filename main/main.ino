#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <dht11.h>
#include <string>
#include <WiFi.h>
#include <WebSocketClient.h>

// LED pins
#define led1Pin 5
#define led2Pin 4
#define led3Pin 2

// PWM configuration
#define freq 5000
#define resolution 8

// PWM channels
#define ledChannel1 5
#define ledChannel2 4
#define ledChannel3 2

// DHT11 pin
#define DHT11PIN 15

// DHT11 sensor
dht11 DHT11;

// Network SSID and password (change these to your own)
const char *ssid = "your_ssid";         
const char *password = "your_password"; 

// Websocket server address (change these to your own)
char path[] = "your_path";
char host[] = "your_host";
const int PORT = your_port;

// Motion sensor pin and variable
int inputPinMotionSens = 21;
bool isSomeonePresent = false;

// PWM duty cycle and variable
int dutyCycle1 = 0;
int dutyCycle2 = 0;
int dutyCycle3 = 0;

double pwm1 = 0;
double pwm2 = 0;
double pwm3 = 0;

// Counter and total variables
int counter = 0;
double totalTemperature = 0;
double totalHumidity = 0;

// Time variables
double currentTime = 0;
double previousTime = 0;

// Websocket client and wifi client
WebSocketClient webSocketClient;
WiFiClient client;

String data;

/**
 * @brief Setup PWM channel
 *
 * @param channel
 * @param pin
 */
void setupPwmChannel(int channel, int pin)
{
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(pin, channel);
}

/**
 * @brief Connect to the wifi network
 */
void connectToWifi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * @brief Connect to the websocket server
 */
void connectToWebSocket()
{
  if (client.connect(host, PORT))
  {
    Serial.println("Connected");
  }
  else
  {
    Serial.println("Connection failed.");
  }

  webSocketClient.path = path;
  webSocketClient.host = host;

  if (webSocketClient.handshake(client))
  {
    Serial.println("Handshake successful");
  }
  else
  {
    Serial.println("Handshake failed.");
  }
}


void setup()
{
  Serial.begin(115200);

  // PWM setup and pin configuration
  setupPwmChannel(ledChannel1, led1Pin);
  setupPwmChannel(ledChannel2, led2Pin);
  setupPwmChannel(ledChannel3, led3Pin);
  // Connection to Wifi
  connectToWifi();
  // Connection to websocket
  connectToWebSocket();
}

void sendData(double temperature, double humidity, bool isSomeonePresent, double pwm1, double pwm2, double pwm3)
{

  DynamicJsonDocument doc(1024);
  doc["macAddress"] = WiFi.macAddress();
  doc["room"] = "room1";
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["presence"] = isSomeonePresent;
  doc["switch_1"] = pwm1;
  doc["switch_2"] = pwm2;
  doc["switch_3"] = pwm3;
  
  String data;
  serializeJson(doc, data);
  webSocketClient.sendData(data);

  Serial.print("Data sent :");
  Serial.println(data);
  data = "";
}

void loop()
{
  String data = "";
  String newdata = "";
  double temperature = 0;
  double humidity = 0;

  if (client.connected())
  {
    DynamicJsonDocument doc(1024);
    DynamicJsonDocument newdoc(1024);

    isSomeonePresent = digitalRead(inputPinMotionSens);

    double chk = DHT11.read(DHT11PIN);

    // -----------------------------------
    //  Ingoing json parameters
    // -----------------------------------
    webSocketClient.getData(newdata);

    deserializeJson(newdoc, newdata);

    // If the incoming data is not empty, and change the duty cycle accordingly
    if (newdoc.containsKey("switch_1"))
    {
      dutyCycle1 = (double)newdoc["switch_1"] * 255;
      pwm1 = (double)newdoc["switch_1"];
      sendData(temperature, humidity, isSomeonePresent, pwm1, pwm2, pwm3);
    }

    else if (newdoc.containsKey("switch_2"))
    {
      dutyCycle2 = (double)newdoc["switch_2"] * 255;
      pwm2 = (double)newdoc["switch_2"];
      sendData(temperature, humidity, isSomeonePresent, pwm1, pwm2, pwm3);
    }

    else if (newdoc.containsKey("switch_3"))
    {
      dutyCycle3 = (double)newdoc["switch_3"] * 255;
      pwm3 = (double)newdoc["switch_3"];
      sendData(temperature, humidity, isSomeonePresent, pwm1, pwm2, pwm3);
    }

    // Change the PWM duty cycle
    ledcWrite(ledChannel1, dutyCycle1);
    ledcWrite(ledChannel2, dutyCycle2);
    ledcWrite(ledChannel3, dutyCycle3);

    // Add the temperature and humidity to the total
    totalTemperature += (double)DHT11.temperature;
    totalHumidity += (double)DHT11.humidity;
    counter++;

    currentTime = millis();

    // Send data every 5 seconds
    if (currentTime - previousTime >= 5000)
    {
      previousTime = currentTime;
      temperature = totalTemperature / counter;
      humidity = totalHumidity / counter;
      counter = 0;

      totalTemperature = 0;
      totalHumidity = 0;

      sendData(temperature, humidity, isSomeonePresent, pwm1, pwm2, pwm3);
    }
  }
  else
  {
    // -----------------------------------------
    // Reconnect to server if connection lost
    // -----------------------------------------

    Serial.println("Client disconnected. Trying to reconnect.");

    if (client.connect(host, PORT))
    {
      Serial.println("Connected");

      if (webSocketClient.handshake(client))
      {
        Serial.println("Handshake successful");
      }
      else
      {
        Serial.println("Handshake failed.");
      }
    }
    else
    {
      Serial.println("Connection failed.");
    }
  }
}
