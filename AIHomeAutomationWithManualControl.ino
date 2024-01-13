// Uncomment the following line to enable serial debug output
#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include <map>

#define WIFI_SSID "iMaK"
#define WIFI_PASS "gate@024"
#define APP_KEY "a2f70f00-5b97-4c70-8d04-f147c3d53ced"
#define APP_SECRET "07903b10-8bde-47a0-8645-2d9688267ef7-f804f37f-840b-4f9e-a103-b969ac87e4e2"

#define device_ID_1 "xxxxxxxxxxxxxxxxxxxxxxxx"
#define device_ID_2 "6585dc7e54ee86d4f7ee3fbc"
#define device_ID_3 "6585dc9854ee86d4f7ee3ff4"
#define device_ID_4 "6585dc4dcf6199223f5cf216"

#define RelayPin1 5  // D1
#define RelayPin2 4  // D2
#define RelayPin3 14 // D5
#define RelayPin4 12 // D6

#define ButtonPin1 10 // SD3
#define ButtonPin2 0  // D3
#define ButtonPin3 13 // D7
#define ButtonPin4 3  // RX

#define wifiLed 16 // D0

#define BAUD_RATE 9600
#define DEBOUNCE_TIME 250

typedef struct
{
  int relayPIN;
  int buttonPIN;
} deviceConfig_t;

std::map<String, deviceConfig_t> devices = {
    {device_ID_1, {RelayPin1, ButtonPin1}},
    {device_ID_2, {RelayPin2, ButtonPin2}},
    {device_ID_3, {RelayPin3, ButtonPin3}},
    {device_ID_4, {RelayPin4, ButtonPin4}}};

typedef struct
{
  String deviceId;
  bool lastButtonState;
  unsigned long lastButtonPress;
} buttonConfig_t;

std::map<int, buttonConfig_t> buttons;

void setupRelays()
{
  for (auto &device : devices)
  {
    int relayPIN = device.second.relayPIN;
    pinMode(relayPIN, OUTPUT);
    digitalWrite(relayPIN, HIGH);
  }
}

void setupButtons()
{
  for (auto &device : devices)
  {
    buttonConfig_t buttonConfig;
    buttonConfig.deviceId = device.first;
    buttonConfig.lastButtonPress = 0;
    buttonConfig.lastButtonState = HIGH;
    int buttonPIN = device.second.buttonPIN;
    buttons[buttonPIN] = buttonConfig;
    pinMode(buttonPIN, INPUT_PULLUP);
  }
}

bool onPowerState(String deviceId, bool &state)
{
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");
  int relayPIN = devices[deviceId].relayPIN;
  digitalWrite(relayPIN, state);
  return true;
}

void handleButtons()
{
  unsigned long actualMillis = millis();
  for (auto &button : buttons)
  {
    unsigned long lastButtonPress = button.second.lastButtonPress;

    if (actualMillis - lastButtonPress > DEBOUNCE_TIME)
    {
      int buttonPIN = button.first;
      bool lastButtonState = button.second.lastButtonState;
      bool buttonState = digitalRead(buttonPIN);

      if (!buttonState)
      { // Assuming LOW when button is pressed
        button.second.lastButtonPress = actualMillis;
        String deviceId = button.second.deviceId;
        int relayPIN = devices[deviceId].relayPIN;
        bool newRelayState = !digitalRead(relayPIN);
        digitalWrite(relayPIN, newRelayState);
        SinricProSwitch &mySwitch = SinricPro[deviceId];
        mySwitch.sendPowerStateEvent(newRelayState);
      }

      button.second.lastButtonState = buttonState;
    }
  }
}

void setupWiFi()
{
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  digitalWrite(wifiLed, LOW);
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro()
{
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, HIGH);

  setupRelays();
  setupButtons();
  setupWiFi();
  setupSinricPro();
}

void loop()
{
  SinricPro.handle();
  handleButtons();
}
