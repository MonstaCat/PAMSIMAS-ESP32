#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FirebaseESP32.h>
#include "esp32-hal-cpu.h"

#define SENSOR1 26 // Pin for the first water flow sensor
#define SENSOR2 27 // Pin for the second water flow sensor

const float calibrationFactor1 = 6.6; // Pulse frequency per L/min for sensor 1
const float calibrationFactor2 = 6.6; // Pulse frequency per L/min for sensor 2
const float tolerance = 0.1; // Tolerance value for comparison

volatile byte pulseCount1;
volatile byte pulseCount2;
unsigned long previousMillis;
boolean leakDetected = false;
unsigned long leakDetectedTime = 0;
boolean recheckLeak = false;
unsigned long recheckTime = 0;
float totalVolume1 = 0.0;
float totalVolume2 = 0.0;

void IRAM_ATTR pulseCounter1() {
    pulseCount1++;
}

void IRAM_ATTR pulseCounter2() {
    pulseCount2++;
}

// Firebase configuration
#define FIREBASE_HOST "https://esp32-database-3c1ad-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "AIzaSyBDSvrxm-tYTErYoxX8AhOzruglgMuZtQ8"

FirebaseData firebaseData;

void connectToWiFi() {
    // Your Wi-Fi credentials
    const char * ssid = "dika";
    const char * password = "876543211";

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);

    connectToWiFi();

    pinMode(SENSOR1, INPUT_PULLUP);
    pinMode(SENSOR2, INPUT_PULLUP);

    pulseCount1 = 0;
    pulseCount2 = 0;
    previousMillis = 0;

    attachInterrupt(digitalPinToInterrupt(SENSOR1), pulseCounter1, FALLING);
    attachInterrupt(digitalPinToInterrupt(SENSOR2), pulseCounter2, FALLING);

    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);
}

void loop() {
    unsigned long currentMillis = millis();

    if (pulseCount1 > 0) {
        // Send is running data to Firebase
        if (Firebase.setBool(firebaseData, "/Status/isRunning", true)) {
            Serial.println("isRunning true data sent to Firebase\n");
        } else {
            Serial.println("Failed to send isRunning true data to Firebase\n");
        }
    } else {
        if (Firebase.setBool(firebaseData, "/Status/isRunning", false)) {
            Serial.println("isRunning false data sent to Firebase\n");
        } else {
            Serial.println("Failed to send isRunning false data to Firebase\n");
        }
    }

    if (currentMillis - previousMillis >= 5000) {
        // Check for leak detection condition after 5 seconds

        float flowRate1 = ((1000.0 / (millis() - previousMillis)) * pulseCount1) / calibrationFactor1; // Calculate flow rate for sensor 1 in L/min
        float flowRate2 = ((1000.0 / (millis() - previousMillis)) * pulseCount2) / calibrationFactor2; // Calculate flow rate for sensor 2 in L/min

        float flowRateTolerance = flowRate2 * tolerance; // Calculate the flow rate tolerance based on sensor 2

        if (flowRate1 > (flowRate2 + flowRateTolerance)) {
            // Leak condition is met
            if (!leakDetected) {
                // If previous leak was not detected, it means a new leak is detected now
                Serial.println("Leak Detected!\n");
                leakDetectedTime = currentMillis;
                recheckLeak = true;
                recheckTime = currentMillis + 5000; // Recheck after 5 seconds

                // Send leak detected data to Firebase
                if (Firebase.setBool(firebaseData, "/Status/LeakDetected", true)) {
                    Serial.println("Leak Detected data sent to Firebase\n");
                } else {
                    Serial.println("Failed to send Leak Detected data to Firebase\n");
                }
            } else {
                // If previous leak was detected, check if the leak persists for 5 seconds
                if (currentMillis - leakDetectedTime >= 5000) {
                    // Leak condition persists for 5 seconds
                    Serial.println("!!!!!!!!!! Leak Confirmed! !!!!!!!!!! \n");
                    recheckLeak = false;

                    // Send leak confirmed data to Firebase
                    if (Firebase.setBool(firebaseData, "/Status/LeakConfirmed", true)) {
                        Serial.println("Leak Confirmed data sent to Firebase\n");
                    } else {
                        Serial.println("Failed to send Leak Confirmed data to Firebase\n");
                    }
                } else if (recheckLeak && currentMillis >= recheckTime) {
                    // Perform recheck after 5 seconds
                    float recheckFlowRate1 = ((1000.0 / (millis() - previousMillis)) * pulseCount1) / calibrationFactor1;
                    float recheckFlowRate2 = ((1000.0 / (millis() - previousMillis)) * pulseCount2) / calibrationFactor2;
                    if (recheckFlowRate1 <= (recheckFlowRate2 + flowRateTolerance)) {
                        // Recheck passed, leak condition resolved
                        Serial.println("Leak Resolved\n");
                        recheckLeak = false;
                        if (Firebase.setBool(firebaseData, "/Status/LeakConfirmed", false)) {
                            Serial.println("Set False Leak Confirmed to Firebase\n");
                        } else {
                            Serial.println("Failed to set False Leak Confirmed data to Firebase\n");
                        }
                        if (Firebase.setBool(firebaseData, "/Status/LeakDetected", false)) {
                            Serial.println("Set False Leak Detected to Firebase\n");
                        } else {
                            Serial.println("Failed to set False Leak Detected data to Firebase\n");
                        }
                    }
                }
            }
            leakDetected = true;
        } else {
            // Leak condition is not met
            if (leakDetected) {
                // If previous leak was detected, it means the leak is resolved now
                Serial.println("Leak Resolved\n");
                leakDetected = false;
                recheckLeak = false;
                if (Firebase.setBool(firebaseData, "/Status/LeakConfirmed", false)) {
                    Serial.println("Set False Leak Confirmed to Firebase\n");
                } else {
                    Serial.println("Failed to set False Leak Confirmed data to Firebase\n");
                }
                if (Firebase.setBool(firebaseData, "/Status/LeakDetected", false)) {
                    Serial.println("Set False Leak Detected to Firebase\n");
                } else {
                    Serial.println("Failed to set False Leak Detected data to Firebase\n");
                }
            }
        }

        // ==================== SENSOR 1 ====================
        if (Firebase.setFloat(firebaseData, "/Sensors/Signal_1", pulseCount1)) {
            Serial.print("Sensor 1 Signal: ");
            Serial.println(pulseCount1);
        } else {
            Serial.println("Failed to send Signal Sensor 1 data to Firebase\n");
        }

        if (Firebase.setFloat(firebaseData, "/Sensors/FlowRate1", flowRate1)) {
            Serial.print("Flow Rate Sensor 1: ");
            Serial.print(flowRate1);
            Serial.println(" L/min");
        } else {
            Serial.println("Failed to send Flow Rate Sensor 1 data to Firebase\n");
        }

        if (Firebase.setFloat(firebaseData, "/Sensors/TotalVolume1", totalVolume1)) {
            float volume1 = flowRate1 * (millis() / (1000.0 * 60.0)); // Flow rate multiplied by time interval (5 seconds)
            totalVolume1 += volume1;
            Serial.print("Total Volume Sensor 1: ");
            Serial.print(totalVolume1);
            Serial.println(" liters \n");
        } else {
            Serial.println("Failed to send Total Volume Sensor 1 data to Firebase\n");
        }
        // ==================== END OF SENSOR 1 ====================

        // ==================== SENSOR 2 ====================
        if (Firebase.setFloat(firebaseData, "/Sensors/Signal_2", pulseCount2)) {
            Serial.print("Sensor 2 Signal: ");
            Serial.println(pulseCount2);
        } else {
            Serial.println("Failed to send Signal Sensor 1 data to Firebase\n");
        }

        if (Firebase.setFloat(firebaseData, "/Sensors/FlowRate2", flowRate2)) {
            Serial.print("Flow Rate Sensor 2: ");
            Serial.print(flowRate2);
            Serial.println(" L/min");
        } else {
            Serial.println("Failed to send Flow Rate Sensor 2 data to Firebase\n");
        }

        if (Firebase.setFloat(firebaseData, "/Sensors/TotalVolume2", totalVolume2)) {
            float volume2 = flowRate2 * 5.0; // Flow rate multiplied by time interval (5 seconds)
            totalVolume2 += volume2;
            Serial.print("Total Volume Sensor 2: ");
            Serial.print(totalVolume2);
            Serial.println(" liters \n");
        } else {
            Serial.println("Failed to send Total Volume Sensor 2 data to Firebase\n");
        }

        // ==================== END OF SENSOR 2 ====================

        // Reset pulse counts for the next measurement interval
        pulseCount1 = 0;
        pulseCount2 = 0;
        previousMillis = currentMillis;
    }
}