/*************************************************************

  This is a simple demo of sending and receiving some data.
  Be sure to check out other examples!
 *************************************************************/

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6pcmxCAh2"
#define BLYNK_TEMPLATE_NAME "Safe Route"
#define BLYNK_AUTH_TOKEN "mBFsTWkrdNe3hyMls_qIMQtOUMHzK3B4"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <Wire.h>
#include <U8g2lib.h>
#include <Keypad.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "OPPO A96";
char pass[] = "b7wphxir";

BlynkTimer timer;

Preferences preferences;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Pin Definitions 
#define BUZZER 23
#define LED 2

#define TRIG_PIN 18
#define ECHO_PIN 19

#define LDR_PIN 34
#define IR_SENSOR_1 17
#define IR_SENSOR_2 4  // Changed from 39 to 4 (valid ESP32 pin)
#define IR_SENSOR_3 35
#define LAMP_1 25
#define LAMP_2 26
#define LAMP_3 27
#define DARK_THRESHOLD 300


const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {13, 12, 14, 33};
byte colPins[COLS] = {32, 15, 5, 16};  // Adjusted for proper keypad operation
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int currentSystem = 0;  // To keep track of the current system

void setup() {
    Serial.begin(9600);
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    pinMode(2, OUTPUT);
    
    pinMode(BUZZER, OUTPUT);
    pinMode(LED, OUTPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT_PULLDOWN);
    pinMode(LDR_PIN, INPUT);
    pinMode(IR_SENSOR_1, INPUT);
    pinMode(IR_SENSOR_2, INPUT);
    pinMode(IR_SENSOR_3, INPUT);
    pinMode(LAMP_1, OUTPUT);
    pinMode(LAMP_2, OUTPUT);
    pinMode(LAMP_3, OUTPUT);

    preferences.begin("mode", false);
    char mode = preferences.getChar("selected_mode", ' ');
    preferences.end();

    u8g2.begin();
    if (mode == 'A') {
        runCollisionPrevention();
    } else if (mode == 'B') {
        runDrowsinessDetection();
    } else if (mode == 'C') {
        runSmartStreetLamp();
    } else {
        displayMenu();
    }

    // Setup a function to be called every second
    timer.setInterval(1000L, myTimerEvent);
}

void loop() {
    Blynk.run();
    timer.run();

    char key = keypad.getKey();
    if (key) {
        preferences.begin("mode", false);
        if (key == '1') {
            preferences.putChar("selected_mode", 'B');
            preferences.end();
            runDrowsinessDetection();
        } else if (key == '2') {
            preferences.putChar("selected_mode", 'A');
            preferences.end();
            runCollisionPrevention();
        } else if (key == '3') {
            preferences.putChar("selected_mode", 'C');
            preferences.end();
            runSmartStreetLamp();
        }
    }
}

void displayMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 20, "1: Drowsiness Detection");
    u8g2.drawStr(10, 40, "2: Collision Prevention");
    u8g2.drawStr(10, 60, "3: Smart Street Lamp");
    u8g2.sendBuffer();
}

void runDrowsinessDetection() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(20, 30, "Drowsiness Mode");
    u8g2.sendBuffer();

    while (true) {
        if (Serial.available() > 0) {
            char input = Serial.read();
            u8g2.clearBuffer();
            if (input == '1') {
                digitalWrite(BUZZER, HIGH);
                digitalWrite(LED, HIGH);
                u8g2.drawStr(20, 30, "DROWSY!!");
                u8g2.drawStr(10, 20, "Please Wake Up!!");
            } else if (input == '0') {
                digitalWrite(BUZZER, LOW);
                digitalWrite(LED, LOW);
                u8g2.drawStr(30, 30, "Driver OK");
            }
            u8g2.sendBuffer();
        }
        if (keypad.getKey() == '#') {
            displayMenu();
            break;
        }
    }
}

void runCollisionPrevention() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(20, 30, "Collision Mode");
    u8g2.sendBuffer();

    while (true) {
        int distance = getDistance();
        Serial.print("Distance: ");
        Serial.println(distance);

        // Send distance to Blynk Virtual Pin V4
        Blynk.virtualWrite(V4, distance);

        if (distance != -1 && distance <= 10) {
            digitalWrite(BUZZER, HIGH);
            digitalWrite(LED, HIGH);
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_ncenB14_tr);
            u8g2.drawStr(10, 30, "Obstacle!");
            u8g2.drawStr(10, 50, "STOP!");
            u8g2.sendBuffer();
            Serial.println("⚠️ Warning! Obstacle detected! ⚠️");
        } else {
            digitalWrite(BUZZER, LOW);
            digitalWrite(LED, LOW);
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.drawStr(30, 30, "Safe Distance");
            u8g2.sendBuffer();
            Serial.println("✅ Safe Distance ✅");
        }

        if (keypad.getKey() == '#') {
            displayMenu();
            break;
        }
        delay(500);
    }
}

int getDistance() {
    long duration;
    int distance;
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Read echo response with a timeout (30ms)
    duration = pulseInLong(ECHO_PIN, HIGH, 30000);
    
    // Convert time to distance in cm
    distance = duration * 0.034 / 2;

    // Validate sensor range (2 cm - 400 cm)
    if (distance < 2 || distance > 400) {
        return -1;  // Return -1 if the distance is out of range
    }

    return distance;
}

void runSmartStreetLamp() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(20, 30, "Smart Street Lamp");
    u8g2.sendBuffer();

    while (true) {
        int darkLevel = analogRead(LDR_PIN);
        int motion1 = !digitalRead(IR_SENSOR_1);
        int motion2 = !digitalRead(IR_SENSOR_2);
        int motion3 = !digitalRead(IR_SENSOR_3);

        Serial.printf("Dark Level: %d | Motion: %d %d %d\n", darkLevel, motion1, motion2, motion3);

        // Send data to Blynk Virtual Pins
        Blynk.virtualWrite(V5, darkLevel);  // Dark Level to V5
        Blynk.virtualWrite(V6, motion1);    // Motion Sensor 1 to V6
        Blynk.virtualWrite(V7, motion2);    // Motion Sensor 2 to V7
        Blynk.virtualWrite(V8, motion3);    // Motion Sensor 3 to V8

        // Turn on lamps based on light and motion detection
        bool lamp1State = (darkLevel > DARK_THRESHOLD && motion1);
        bool lamp2State = (darkLevel > DARK_THRESHOLD && motion2);
        bool lamp3State = (darkLevel > DARK_THRESHOLD && motion3);

        digitalWrite(LAMP_1, lamp1State);
        digitalWrite(LAMP_2, lamp2State);
        digitalWrite(LAMP_3, lamp3State);

        // Update OLED display with lamp statuses
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB08_tr);

        if (lamp1State) {
            u8g2.drawStr(10, 40, "Lamp 1: ON");
        } else {
            u8g2.drawStr(10, 40, "Lamp 1: OFF");
        }

        if (lamp2State) {
            u8g2.drawStr(10, 50, "Lamp 2: ON");
        } else {
            u8g2.drawStr(10, 50, "Lamp 2: OFF");
        }

        if (lamp3State) {
            u8g2.drawStr(10, 60, "Lamp 3: ON");
        } else {
            u8g2.drawStr(10, 60, "Lamp 3: OFF");
        }

        u8g2.sendBuffer();

        if (keypad.getKey() == '#') {
            displayMenu();
            break;
        }
        delay(100);
    }
}

void myTimerEvent() {
    // Send uptime to Virtual Pin V2
    Blynk.virtualWrite(V2, millis() / 1000);
}
// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0) {
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();

  // Update state
  Blynk.virtualWrite(V1, value);
  digitalWrite(2, value ? HIGH : LOW);
}

// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED() {
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}
