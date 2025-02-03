/**
 * A basic example of using the EventButton.
 *
 * When the button is pressed the inbuilt LED on pin 13
 * will turn on, when released will turn off.
 *
 * Unlike the standard Examples->Digital->Button example, our 
 * button is connected between pin 2 and GND because we 
 * use INPUT_PULLUP internally.
 *
 */
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <EventButton.h>
#include <GaugeMinimal.h>

const uint8_t buttonTopLeftPin = 25;  // the number of the pushbutton pin
const uint8_t buttonBottomLeftPin = 26;  // the number of the pushbutton pin
const uint8_t buttonTopRightPin = 14;  // the number of the pushbutton pin
const uint8_t buttonBottomRightPin = 27;  // the number of the pushbutton pin

#define LONG_CLICK_DURATION 1000 // 1 second for long click

typedef struct struct_button {
    uint8_t flag;
    uint8_t button;
    uint8_t press_type;
} struct_button;

typedef struct struct_speedo {
  uint8_t flag;
  uint8_t speed_kmph;
  uint8_t speed_mph;
  uint8_t fuel_perc;
}struct_speedo;

struct_speedo SpeedoData;
struct_button ButtonData;
struct_startup StartupData;

esp_now_peer_info_t peerInfo;

uint8_t speedoAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Replace with speedo ESP32 MAC
uint8_t levelsAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Replace with levels ESP32 MAC
uint8_t weatherAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Replace with weather ESP32 MAC

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Send Success" : "Send Failed");
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  if (len == 1) {
    uint8_t new_channel = incomingData[0];
    esp_wifi_set_channel(new_channel, WIFI_SECOND_CHAN_NONE);
    Serial.printf("Channel updated to: %d\n", new_channel);
  }
}

void sendButtonEvent(uint8_t button_id, uint8_t press_type) {
    ButtonData.button = button_id;
    ButtonData.press_type = press_type;
    esp_now_send(speedoAddress, (uint8_t *)&ButtonData, sizeof(ButtonData));
    esp_now_send(levelsAddress, (uint8_t *)&ButtonData, sizeof(ButtonData));
    esp_now_send(weatherAddress, (uint8_t *)&ButtonData, sizeof(ButtonData));
}

/**
 * A function to handle the events
 * Can be called anything but requires InputEventType and
 * EventButton& defined as parameters.
 */
void onButtonEvent(InputEventType event, EventButton& btn) {
  uint8_t button_id = btn.getInputId();
  
  switch (event) {
    case InputEventType::CLICKED:
      sendButtonEvent(button_id, CLICK_EVENT_CLICK);
      Serial.print("Click");
      break;
    case InputEventType::DOUBLE_CLICKED:
      sendButtonEvent(button_id, CLICK_EVENT_DOUBLE);
      Serial.print("Double");
      break;
    case InputEventType::LONG_PRESS:
      sendButtonEvent(button_id, CLICK_EVENT_HOLD);
      Serial.print("Hold");
      break;
  }
     
}

EventButton buttonTopLeft(buttonTopLeftPin);
EventButton buttonBottomLeft(buttonBottomLeftPin);
EventButton buttonTopRight(buttonTopRightPin);
EventButton buttonBottomRight(buttonBottomRightPin);

void init_wifi(void) {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed");
        return;
    }
    
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

    peerInfo.channel = 0; // Default channel
    peerInfo.encrypt = false;

    memcpy(peerInfo.peer_addr, speedoAddress, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    memcpy(peerInfo.peer_addr, levelsAddress, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    memcpy(peerInfo.peer_addr, weatherAddress, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    Serial.println("ESP-NOW Initialized");

    delay(3000); // not usually recommended but fine here as nothing to block

    // send the sync startup flag to all screens
    StartupData.flag = FLAG_STARTUP;
    esp_now_send(speedoAddress, (uint8_t *)&StartupData, sizeof(StartupData));
    esp_now_send(levelsAddress, (uint8_t *)&StartupData, sizeof(StartupData));
    esp_now_send(weatherAddress, (uint8_t *)&StartupData, sizeof(StartupData));
    Serial.println("Startup flag sent");
}


void setup() {
  Serial.begin(115200);

  init_wifi();
  
  ButtonData.flag = 2;

  SpeedoData.flag = 0;
  SpeedoData.fuel_perc = 60;

  buttonTopLeft.begin();
  buttonBottomLeft.begin();
  buttonTopRight.begin();
  buttonBottomRight.begin();

  buttonTopLeft.setInputId(BUTTON_SETTING);
  buttonBottomLeft.setInputId(BUTTON_MODE);
  buttonTopRight.setInputId(BUTTON_BRIGHTNESS_UP);
  buttonBottomRight.setInputId(BUTTON_BRIGHTNESS_DOWN);

  buttonTopLeft.setCallback(onButtonEvent);
  buttonBottomLeft.setCallback(onButtonEvent);
  buttonTopRight.setCallback(onButtonEvent);
  buttonBottomRight.setCallback(onButtonEvent);

  buttonTopLeft.enableLongPressRepeat(false);
  buttonBottomLeft.enableLongPressRepeat(false);
  buttonTopRight.enableLongPressRepeat(false);
  buttonBottomRight.enableLongPressRepeat(false);

  buttonTopLeft.setLongClickDuration(LONG_CLICK_DURATION);
  buttonBottomLeft.setLongClickDuration(LONG_CLICK_DURATION);
  buttonTopRight.setLongClickDuration(LONG_CLICK_DURATION);
  buttonBottomRight.setLongClickDuration(LONG_CLICK_DURATION);
}

void loop() {
  buttonTopLeft.update();
  buttonBottomLeft.update();
  buttonTopRight.update();
  buttonBottomRight.update();
}