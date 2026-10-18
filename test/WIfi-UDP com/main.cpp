#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WiFiUdp.h>

String ssid = "Airtel_Karn";
String password = "ojas1234";

const int udpPort = 8888;
bool udpStarted = false;

WiFiUDP udp;


#define SERVOMIN   90
#define SERVOMAX   500
#define SERVO_FREQ 50

#define START_MARKER 0xAA
#define END_MARKER   0x55

#define LED_PIN    48
#define NUM_PIXELS  1

const int btnPin = 18;
const int servoBreakpin = 17;
const int servoRstPin = 16;

volatile bool brkBtnPressed = false;

int fingerCount = 1;
int analogPins[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

bool isSweeping = false;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);


struct __attribute__((packed)) Packet {
    uint8_t cmd;
    uint8_t isSweeping;
    uint8_t speedDelay;
    uint8_t crc;
};

#pragma pack(push,1)
struct wifiPacket {
    uint8_t cmd;
    uint8_t credentials[64];
    uint8_t crc;
};
#pragma pack(pop)



uint8_t wifiBuffer[sizeof(Packet)];
uint8_t rxBuffer[sizeof(wifiPacket)];   
size_t rxIndex = 0;
bool receiving = false;


uint8_t calculateCRC(const uint8_t* data, size_t len) {

    uint8_t crc = 0;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
    }

    return crc;
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


void servoSweep(int pinRange, int speedDelay) {

    for (int pulseWidth = SERVOMIN;
         pulseWidth <= SERVOMAX;
         pulseWidth += 5) {

        for (int pin = 0; pin < pinRange; pin++) {
            pwm.setPWM(pin, 0, pulseWidth);
        }

        delay(speedDelay);
    }

    delay(500);


    for (int pulseWidth = SERVOMAX;
         pulseWidth >= SERVOMIN;
         pulseWidth -= 5) {

        for (int pin = 0; pin < pinRange; pin++) {
            pwm.setPWM(pin, 0, pulseWidth);
        }

        delay(speedDelay);
    }

    delay(500);
}

void ledFade(int r, int g, int b, int speed,int brightness) { 
  for (int i = 0; i <= brightness; i++) {
    int currentR = (r * i) / brightness;
    int currentG = (g * i) / brightness;
    int currentB = (b * i) / brightness;
    
    neopixelWrite(LED_PIN, currentR, currentG, currentB); 
    delay(speed); 
  }

  for (int i = brightness; i >= 0; i--) {
    int currentR = (r * i) / brightness;
    int currentG = (g * i) / brightness;
    int currentB = (b * i) / brightness;
    
    neopixelWrite(LED_PIN, currentR, currentG, currentB); 
    delay(speed); 
  }
  
  delay(250);
}

void ledBlink(int r, int g, int b, int times, int speed, int brightness) {

    int targetR = (r * brightness) / 255;
    int targetG = (g * brightness) / 255;
    int targetB = (b * brightness) / 255;

    for (int i = 0; i < times; i++) {
        neopixelWrite(LED_PIN, targetR, targetG, targetB);
        delay(speed);
        neopixelWrite(LED_PIN, 0, 0, 0);
        delay(speed);
    }
}
void wificonnectingLED(int r, int g, int b, int speed, int brightness) {

    int targetR = (r * brightness) / 255;
    int targetG = (g * brightness) / 255;
    int targetB = (b * brightness) / 255;

    neopixelWrite(LED_PIN, targetR, targetG, targetB);
    delay(speed);
    neopixelWrite(LED_PIN, 0, 0, 0);
    delay(speed);
}

void connectWiFi() {
    udpStarted = false;
    Serial.println("Connecting...");

    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        Serial.print(".");
        wificonnectingLED(255, 70, 70, 70, 70); 
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        ledBlink(255, 255, 255, 2, 170, 70);
        
        Serial.println("CONNECTED");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("ESP32 MAC Address: ");
        Serial.println(WiFi.macAddress());

        if (udp.begin(udpPort)) {
            udpStarted = true;
            Serial.println("UDP started successfully");
        } else {
            Serial.println("UDP FAILED TO START");
        }
    }
}



void handleUDP(){

     if(udpStarted){
        int packetSize = udp.parsePacket();


        if (packetSize > 0) {

            udp.read(wifiBuffer, packetSize);

            uint8_t packID = wifiBuffer[0];


            if (packID == 0x01 && packetSize == sizeof(Packet)) {

                Packet* pkt =
                    reinterpret_cast<Packet*>(wifiBuffer);


                if (pkt->crc ==
                    calculateCRC(wifiBuffer, sizeof(Packet) - 1)) {


                    if (pkt->isSweeping == 0x01) {

                        servoSweep(
                            fingerCount,
                            pkt->speedDelay
                        );
                    }
                }
            }
        }
    }

}


void handleWifiConnection(){
        while (Serial.available() > 0) {

    uint8_t byteIn = Serial.read();

    if (!receiving) {

        if (byteIn == START_MARKER) {
            receiving = true;
            rxIndex = 0;
        }

    } else {

        if (byteIn == END_MARKER) {

            uint8_t packetID = rxBuffer[0];

            if (packetID == 0x03 &&
                rxIndex == sizeof(wifiPacket)) {

                wifiPacket* pkt =
                    reinterpret_cast<wifiPacket*>(rxBuffer);

                if (pkt->crc ==
                    calculateCRC(rxBuffer, sizeof(wifiPacket) - 1)) {

                    int separator = -1;

                    for (int i = 0; i < 64; i++) {
                        if (pkt->credentials[i] == '\0') {
                            separator = i;
                            break;
                        }
                    }

                    if (separator != -1) {

                        ssid = "";
                        password = "";

                        for (int i = 0; i < separator; i++) {
                            ssid += (char)pkt->credentials[i];
                        }

                        for (int i = separator + 1; i < 64; i++) {
                            if (pkt->credentials[i] == '\0')
                                break;

                            password += (char)pkt->credentials[i];
                        }

                        connectWiFi();
                    }
                }
            }

            receiving = false;
            rxIndex = 0;
        }

        else {

            if (rxIndex < sizeof(rxBuffer)) {
                rxBuffer[rxIndex++] = byteIn;
            }
            else {
                receiving = false;
                rxIndex = 0;
            }
        }
    }
}
}


void setup() {
    
    Serial.begin(115200);
    Wire.begin(8,9);
    
    connectWiFi();
    Serial.println("UDP listening on port 8888");


    pwm.begin();
    pwm.setPWMFreq(SERVO_FREQ);

    delay(10);

    pinMode(btnPin, INPUT_PULLUP);
    pinMode(servoRstPin, INPUT_PULLUP);
    pinMode(servoBreakpin, OUTPUT);

    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED,HIGH);
}



void loop() {

    handleUDP();
    handleWifiConnection();
}