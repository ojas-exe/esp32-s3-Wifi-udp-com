#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>

#define SERVOMIN   90
#define SERVOMAX   500
#define SERVO_FREQ 50

#define LED_PIN    48
#define NUM_PIXELS  1

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);


int myGlobalSerialNumber = 100;
const int ISR_PIN = 11;


static TaskHandle_t readSerialTask = NULL;
static TaskHandle_t ledBlinkTask = NULL;

struct LED_CONFIG {
    int r;
    int g;
    int b;
    int speed;
    int brightness;
};

LED_CONFIG ledconfig1 = {255, 70, 70, myGlobalSerialNumber, 100};

void servoSweep(int pinRange, int speedDelay) {

    for (int pulseWidth = SERVOMIN;
         pulseWidth <= SERVOMAX;
         pulseWidth += 5) {

        for (int pin = 0; pin < pinRange; pin++) {
            pwm.setPWM(pin, 0, pulseWidth);
        }

        vTaskDelay(speedDelay/portTICK_PERIOD_MS);
    }

    vTaskDelay(500/portTICK_PERIOD_MS);


    for (int pulseWidth = SERVOMAX;
         pulseWidth >= SERVOMIN;
         pulseWidth -= 5) {

        for (int pin = 0; pin < pinRange; pin++) {
            pwm.setPWM(pin, 0, pulseWidth);
        }

        vTaskDelay(speedDelay/portTICK_PERIOD_MS);
    }

    vTaskDelay(500/portTICK_PERIOD_MS);
}

void ledFade(int r, int g, int b, int speed,int brightness) { 
  for (int i = 0; i <= brightness; i++) {
    int currentR = (r * i) / brightness;
    int currentG = (g * i) / brightness;
    int currentB = (b * i) / brightness;
    
    neopixelWrite(LED_PIN, currentR, currentG, currentB); 
    vTaskDelay(speed/portTICK_PERIOD_MS); 
  }

  for (int i = brightness; i >= 0; i--) {
    int currentR = (r * i) / brightness;
    int currentG = (g * i) / brightness;
    int currentB = (b * i) / brightness;
    
    neopixelWrite(LED_PIN, currentR, currentG, currentB); 
    vTaskDelay(speed/portTICK_PERIOD_MS);
  }
  
  vTaskDelay(250/portTICK_PERIOD_MS);
}

void ledFadeInfinite(int r, int g, int b, int speed,int brightness) { 
  
    while(true){
        for (int i = 0; i <= brightness; i++) {
        int currentR = (r * i) / brightness;
        int currentG = (g * i) / brightness;
        int currentB = (b * i) / brightness;
        
        neopixelWrite(LED_PIN, currentR, currentG, currentB); 
        vTaskDelay(speed/portTICK_PERIOD_MS); 
    }

        for (int i = brightness; i >= 0; i--) {
            int currentR = (r * i) / brightness;
            int currentG = (g * i) / brightness;
            int currentB = (b * i) / brightness;
            
            neopixelWrite(LED_PIN, currentR, currentG, currentB); 
            vTaskDelay(speed/portTICK_PERIOD_MS);
        }

      vTaskDelay(250/portTICK_PERIOD_MS);
    }
  

}

void ledBlink(int r, int g, int b, int times, int speed, int brightness) {

    int targetR = (r * brightness) / 255;
    int targetG = (g * brightness) / 255;
    int targetB = (b * brightness) / 255;

    for (int i = 0; i < times; i++) {
        neopixelWrite(LED_PIN, targetR, targetG, targetB);
        vTaskDelay(speed/portTICK_PERIOD_MS);;
        neopixelWrite(LED_PIN, 0, 0, 0);
        vTaskDelay(speed/portTICK_PERIOD_MS);;
    }
}

void ledBlinkInfinity(void *param) {

    LED_CONFIG *led = (LED_CONFIG *)param;


    int targetR = (led->r * led->brightness) / 255;
    int targetG = (led->g * led->brightness) / 255;
    int targetB = (led->b * led->brightness) / 255;

    while(1){
        neopixelWrite(LED_PIN, targetR, targetG, targetB);
        vTaskDelay(led->speed/portTICK_PERIOD_MS);
        neopixelWrite(LED_PIN, 0, 0, 0);
        vTaskDelay(led->speed/portTICK_PERIOD_MS);
    }
}


void servoWrite(uint8_t channel, int angle) {

    if (angle < 0) {
        angle = 0;
    }

    if (angle > 180) {
        angle = 180;
    }

    int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);

    pwm.setPWM(channel, 0, pulse);
}

void ReadSerial(void *param){
    while(true){
        if(Serial.available()){
            int num = Serial.parseInt();
            ledconfig1.speed = num;
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void ISR(void *param){
    while(true){
        if(digitalRead(ISR_PIN) == LOW){
            vTaskSuspend(ledBlinkTask);
            vTaskDelay(300/portTICK_PERIOD_MS);
            ledFadeInfinite(255, 0, 0, 4, 100);
            vTaskDelay(300/portTICK_PERIOD_MS);
            vTaskResume(ledBlinkTask);
        }
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}


void setup() {
    
    Serial.begin(115200);
    Wire.begin(8,9);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    pinMode(ISR_PIN,INPUT_PULLUP);

    xTaskCreatePinnedToCore(
        ledBlinkInfinity,
        "LedBlink",
        2048,
        &ledconfig1,
        1,
        &ledBlinkTask,
        1

    );

    xTaskCreatePinnedToCore(
        ReadSerial,
        "Read serial",
        2048,
        NULL,
        1,
        &readSerialTask,
        1

    );

    xTaskCreatePinnedToCore(
    ISR,
    "ISR",
    2048,
    NULL,
    25,
    NULL,
    0
);
    
};


void loop() {

};