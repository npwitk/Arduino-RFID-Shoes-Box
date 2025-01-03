#include <SPI.h>
#include <RFID.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Constants
const int I2C_ADDR = 0x27;
const int BACKLIGHT_PIN = 3;
const int SS_PIN = 10;
const int RST_PIN = 9;
const int BUZZER_PIN = 4;
const int NUM_LOCKERS = 6;
const int LED_PINS[] = {13, 8, 7, 6, A0, A1, A2};

// RFID card struct
struct RFIDCard {
    byte uid[5];
    const char* name;
    int id;
};

// Locker struct
struct Locker {
    bool occupied;
    int userId;
};

// User database
const RFIDCard USERS[] = {
    {{42, 187, 106, 22, 237}, "Google", 1},
    {{90, 176, 88, 21, 167}, "Pop", 2},
    {{122, 167, 101, 22, 174}, "Tamp", 3},
    {{170, 32, 77, 21, 210}, "Up", 4},
    {{74, 32, 99, 21, 28}, "Puan", 5},
    {{231, 10, 34, 48, 255}, "Admin", 0}  // Admin card
};

// Initialize components
SoftwareSerial UnoSerial(2, 3);
LiquidCrystal_I2C lcd(I2C_ADDR, 2, 1, 0, 4, 5, 6, 7);
RFID rfid(SS_PIN, RST_PIN);

// Global state
Locker lockers[NUM_LOCKERS];
bool pickupMode = false;
int activeUserId = -1;

void setup() {
    setupPins();
    setupCommunication();
    setupLCD();
}

void loop() {
    updateLockerLEDs();
    
    if (!rfid.isCard() || !rfid.readCardSerial()) {
        return;
    }
    
    const RFIDCard* user = identifyUser(rfid.serNum);
    if (!user) {
        showInvalidCard();
        return;
    }
    
    if (user->id == 0) {  // Admin card
        resetSystem();
        return;
    }
    
    processUserAccess(*user);
    rfid.halt();
}

void processUserAccess(const RFIDCard& user) {
    if (!hasActiveLocker(user.id)) {
        assignLocker(user);
    } else {
        initiatePickup(user);
    }
}

void assignLocker(const RFIDCard& user) {
    int lockerNum = findEmptyLocker();
    if (lockerNum == -1) {
        showMessage("FULL", 2000);
        return;
    }
    
    lockers[lockerNum].occupied = true;
    lockers[lockerNum].userId = user.id;
    
    String msg = "Locker " + String(lockerNum + 1);
    showMessage(msg, 2000);
    playSuccessTone();
    
    UnoSerial.println(String(lockerNum + 1));
}

void initiatePickup(const RFIDCard& user) {
    showMessage("Picking up...", 1000);
    int lockerNum = findUserLocker(user.id);
    
    if (lockerNum != -1) {
        releaseLocker(lockerNum);
        showMessage("THANK YOU", 1000);
        playSuccessTone();
    }
}

// Helper functions
void playSuccessTone() {
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
    digitalWrite(BUZZER_PIN, HIGH);
}

void showMessage(const String& msg, int duration) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(msg);
    delay(duration);
    lcd.clear();
}

const RFIDCard* identifyUser(byte* cardUID) {
    for (const auto& user : USERS) {
        if (memcmp(cardUID, user.uid, 5) == 0) {
            return &user;
        }
    }
    return nullptr;
}

void resetSystem() {
    showMessage("Resetting...", 1000);
    for (int i = 0; i < NUM_LOCKERS; i++) {
        lockers[i] = {false, -1};
    }
    showMessage("Done!", 1000);
    playSuccessTone();
}

void setupPins() {
    pinMode(BUZZER_PIN, OUTPUT);
    for (int pin : LED_PINS) {
        pinMode(pin, OUTPUT);
    }
    pinMode(2, INPUT);
    pinMode(3, OUTPUT);
}

void setupCommunication() {
    Serial.begin(9600);
    UnoSerial.begin(57600);
    SPI.begin();
    rfid.init();
}

void setupLCD() {
    lcd.begin(16, 2);
    lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
    lcd.setBacklight(HIGH);
    lcd.home();
}

void updateLockerLEDs() {
    for (int i = 0; i < NUM_LOCKERS; i++) {
        digitalWrite(LED_PINS[i], lockers[i].occupied ? HIGH : LOW);
    }
}