
#include <Arduino.h>
#include <Ticker.h>
#include <FastLED.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <wifi_config.h>

#define HOSTNAME "photobox"


#define COUNTDOWN_LENGTH 3000
#define FOCUS_TIME 500
#define LOOP_INTERVAL 100
#define MAX_PHOTO_COUNT 2

#define NUM_LEDS 12
#define BRIGHTEN_RATE 20

#define FOCUS_PIN D1
#define TRIGGER_PIN D3
#define BUTTON_PIN D2
#define LED_PIN 4

enum State {
        STATE_IDLE,
        STATE_COUNTDOWN,
        STATE_PHOTO
};

Ticker mainTick;
CRGB ledRing[NUM_LEDS];

State mainState = STATE_IDLE;

int currentLed = 0;
long lastIdleAninmation = 0;
bool brighten = false;


int buttonState = 0;
int countdown = 0;
long countDownStart = 0;

int photoCount = 0;

//int two_pi_fo = 2 *

bool focusActive = false;







void onButton() {
        if(mainState == STATE_IDLE) {
                Serial.println("Triggered");
                mainState = STATE_COUNTDOWN;
                countDownStart = millis();

        }


}


void onTick() {
        long currentTime = millis();
        if(mainState == STATE_PHOTO) {
                //do nothing here
                return;
        }

        if(mainState == STATE_IDLE) {
                if((currentTime - lastIdleAninmation > 5000) || (currentLed != 0)) {

                      if(currentTime - lastIdleAninmation < 200  ) {
                        return;
                      }
                        for(int i = 0; i < NUM_LEDS; i++ ) {
                                ledRing[i] = (i == currentLed) ? CRGB::Red : CRGB::Green;
                        }
                        currentLed++;
                        if(currentLed == NUM_LEDS) {
                                //finish
                                currentLed = 0;
                        }

                        lastIdleAninmation = currentTime;
                        FastLED.show();
                }
                else {
                        FastLED.showColor(CRGB::Green);
                }


                return;

        }

        if(mainState == STATE_COUNTDOWN) {
                FastLED.setBrightness(255);
                currentTime = currentTime - countDownStart;

                if((currentTime % 1000) > 500) {
                        FastLED.showColor(CRGB::Blue);
                }
                else {
                        FastLED.showColor(CRGB::Black);
                }

                if(currentTime  > COUNTDOWN_LENGTH - FOCUS_TIME ) {
                        //Trigger camera focus
                        if(!focusActive) {
                                Serial.println("focusing");
                                focusActive = true;
                                digitalWrite(FOCUS_PIN, LOW);
                        }
                }

                if(currentTime  > COUNTDOWN_LENGTH ) {
                        mainState = STATE_PHOTO;
                        return;
                }
        }

}




void setup()
{
        Serial.begin(115200);

        pinMode(BUTTON_PIN, INPUT_PULLUP);
        pinMode(FOCUS_PIN, OUTPUT);
        pinMode(TRIGGER_PIN, OUTPUT);

        digitalWrite(FOCUS_PIN, HIGH);
        digitalWrite(TRIGGER_PIN, HIGH);



        attachInterrupt(BUTTON_PIN, onButton, FALLING);
        mainTick.attach(0.01, onTick);




        FastLED.addLeds<NEOPIXEL, LED_PIN>(ledRing, NUM_LEDS);

        FastLED.showColor(CRGB::Yellow);

        String hostname(HOSTNAME);
        WiFi.hostname(hostname);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, wpa_key);

        while (WiFi.waitForConnectResult() != WL_CONNECTED) {
                Serial.println("Connection Failed! Rebooting...");
                FastLED.showColor(CRGB::Red);
                delay(1000);
                //ESP.restart();
        }


        ArduinoOTA.setHostname((const char *)WiFi.hostname().c_str());
        ArduinoOTA.onStart([]() {
                mainState = STATE_IDLE;
                mainTick.detach();

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("Start updating... ");
        });
        ArduinoOTA.onEnd([]() {
                Serial.println("\nEnd");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
                int scaled = progress / (total / NUM_LEDS);
                for (int i = 0; i < NUM_LEDS; i++ ) {
                        ledRing[i] = (i < scaled) ? CRGB::Yellow : CRGB::Black;
                }
                FastLED.show();
        });
        ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

        ArduinoOTA.begin();

        Serial.println(" ");
        Serial.println(" ");
        Serial.println("-------------------");
        Serial.println("Setup done");

        FastLED.showColor(CRGB::Green);




}





void loop()
{
        ArduinoOTA.handle();


        if(mainState == STATE_PHOTO) {
                //Trigger photo

                FastLED.showColor(CRGB::White);

                Serial.println("cheese!!!");
                digitalWrite(TRIGGER_PIN, LOW);
                delay(100);
                digitalWrite(TRIGGER_PIN, HIGH);
                digitalWrite(FOCUS_PIN, HIGH);

                FastLED.showColor(CRGB::Black);

                focusActive = false;
                if(photoCount < MAX_PHOTO_COUNT) {
                        mainState = STATE_COUNTDOWN;
                        countDownStart = millis()-2000;
                        photoCount++;
                }
                else {
                        mainState = STATE_IDLE;
                        photoCount = 0;

                }
        }

        delay(2);
}
