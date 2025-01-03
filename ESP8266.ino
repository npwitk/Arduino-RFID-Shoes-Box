#include <ESP8266WiFi.h>
#include <TridentTD_LineNotify.h>
#include <SoftwareSerial.h>

// Configuration
const char* WIFI_SSID = "WIFI_NAME";  // Replace with your WiFi name
const char* WIFI_PASSWORD = "WIFI_PASS";  // Replace with your WiFi password 
const char* LINE_TOKEN = "LINE_TOKEN";  // Replace with your LINE token

struct User {
    const char* name;
    const char* id;
};

const User USERS[] = {
    {"Google", "1"},
    {"Pop", "2"},
    {"Tamp", "3"},
    {"Up", "4"},
    {"Puan", "5"}
};

const int NUM_LOCKERS = 6;
SoftwareSerial NodeSerial(D3, D4);

void setupWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("WiFi connecting to %s\n", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(400);
    }
}

void setup() {
    Serial.begin(9600);
    NodeSerial.begin(57600);
    Serial.println("NodeMCU/ESP8266 Run");
    
    setupWiFi();
    LINE.setToken(LINE_TOKEN);
}

void sendLockerNotification(const char* userName, int lockerNum) {
    String msg = "Your shoes have been put in locker number: " + 
                 String(lockerNum) + " by " + userName;
    LINE.notify(msg);
}

void sendPickupNotification(const char* userName) {
    String msg = userName + String(" has already taken the shoes out of the box.");
    LINE.notify(msg);
}

void handleSystemReset() {
    LINE.notify("The machine has reset the system due to an unauthorized attempt to get the shoes out.");
}

void loop() {
    if (!NodeSerial.available()) return;
    
    int code = NodeSerial.parseInt();
    if (code <= 0) return;
    
    // Handle regular locker assignments (codes 1-30)
    if (code <= NUM_LOCKERS * 5) {
        int userIndex = (code - 1) / NUM_LOCKERS;
        int lockerNum = (code - 1) % NUM_LOCKERS + 1;
        
        if (userIndex < sizeof(USERS)/sizeof(USERS[0])) {
            sendLockerNotification(USERS[userIndex].name, lockerNum);
        }
        return;
    }
    
    // Handle pickups (codes 31-35)
    if (code <= 35) {
        int userIndex = code - 31;
        if (userIndex < sizeof(USERS)/sizeof(USERS[0])) {
            sendPickupNotification(USERS[userIndex].name);
        }
        return;
    }
    
    // Handle system resets (codes 36-37)
    if (code <= 37) {
        handleSystemReset();
    }
}