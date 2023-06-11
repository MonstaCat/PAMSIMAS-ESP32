#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FirebaseESP32.h>

#define SENSOR1 26
#define SENSOR2 27

long currentMillis1 = 0;
long previousMillis1 = 0;

long currentMillis2 = 0;
long previousMillis2 = 0;

int interval = 1000;
float calibrationFactor = 6.6;

volatile byte pulseCount1;
byte pulse1Sec1 = 0;
float flowRate1;
unsigned int flowMilliLitres1;
unsigned long totalMilliLitres1;

volatile byte pulseCount2;
byte pulse1Sec2 = 0;
float flowRate2;
unsigned int flowMilliLitres2;
unsigned long totalMilliLitres2;

boolean leakDetected = false;
unsigned long leakDetectedTime = 0;
boolean recheckLeak = false;
unsigned long recheckTime = 0;

void IRAM_ATTR pulseCounter1()
{
	pulseCount1++;
}

void IRAM_ATTR pulseCounter2()
{
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

void setup()
{
	Serial.begin(115200);

	connectToWiFi();

	pinMode(SENSOR1, INPUT_PULLUP);
	pinMode(SENSOR2, INPUT_PULLUP);
	
	pulseCount1 = 0;
	flowRate1 = 0.0;
	flowMilliLitres1 = 0;
	totalMilliLitres1 = 0;

	pulseCount2 = 0;
	flowRate2 = 0.0;
	flowMilliLitres2 = 0;
	totalMilliLitres2 = 0;

	previousMillis1 = 0;
	previousMillis2 = 0;

	Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);

	attachInterrupt(digitalPinToInterrupt(SENSOR1), pulseCounter1, FALLING);
	attachInterrupt(digitalPinToInterrupt(SENSOR2), pulseCounter2, FALLING);
}

void loop()
{
	currentMillis1 = millis();
	currentMillis2 = millis();

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

	if (currentMillis1 - previousMillis1 > interval) {
		
		pulse1Sec1 = pulseCount1;
		pulseCount1 = 0;

		flowRate1 = ((1000.0 / (millis() - previousMillis1)) * pulse1Sec1) / calibrationFactor;

		previousMillis1 = millis();

		flowMilliLitres1 = (flowRate1 / 60) * 1000;

		totalMilliLitres1 += flowMilliLitres1;

		Serial.print("Output Liquid Quantity 1: ");
		Serial.print(totalMilliLitres1);
		Serial.print("mL / ");
		Serial.print(totalMilliLitres1 / 1000);
		Serial.println("L\n");

		// ==================== SENSOR 1 ====================
        if (Firebase.setInt(firebaseData, "/Sensors/pulseCount1", pulse1Sec1)) {
            // Serial.print("Sensor 1 Pulse Count: ");
            // Serial.print(pulse1Sec1);
			// Serial.print("\t");
        } else {
            // Serial.println("Failed to send Pulse Count 1 data to Firebase\n");
        }

        if (Firebase.setFloat(firebaseData, "/Sensors/FlowRate1", flowRate1)) {
            // Serial.print("Flow rate 1: ");
			// Serial.print(int(flowRate1)); 
			// Serial.print("L/min");
			// Serial.print("\t");
        } else {
            // Serial.println("Failed to send Flow Rate Sensor 1 data to Firebase\n");
        }

		if (Firebase.setFloat(firebaseData, "/Sensors/totalMilliLitres1", totalMilliLitres1)) {
            // Serial.print("Output Liquid Quantity 1: ");
			// Serial.print(totalMilliLitres1);
			// Serial.print("mL / ");
			// Serial.print(totalMilliLitres1 / 1000);
			// Serial.println("L\n");
		} else {
			// Serial.println("Failed to send Total Mili Litres 1 data to Firebase\n");
		}
		// ==================== END OF SENSOR 1 ====================
	}

	if (currentMillis2 - previousMillis2 > interval) {
		
		pulse1Sec2 = pulseCount2;
		pulseCount2 = 0;

		flowRate2 = ((1000.0 / (millis() - previousMillis2)) * pulse1Sec2) / calibrationFactor;

		previousMillis2 = millis();

		flowMilliLitres2 = (flowRate2 / 60) * 1000;

		totalMilliLitres2 += flowMilliLitres2;

		Serial.print("Output Liquid Quantity 2: ");
		Serial.print(totalMilliLitres2);
		Serial.print("mL / ");
		Serial.print(totalMilliLitres2 / 1000);
		Serial.println("L\n");

		// ==================== SENSOR 2 ====================
        if (Firebase.setInt(firebaseData, "/Sensors/pulseCount2", pulse1Sec2)) {
            // Serial.print("Sensor 2 Pulse Count: ");
            // Serial.print(pulse1Sec2);
			// Serial.print("\t");
        } else {
            // Serial.println("Failed to send Pulse Count 2 data to Firebase\n");
        }

        if (Firebase.setFloat(firebaseData, "/Sensors/FlowRate2", flowRate2)) {
            // Serial.print("Flow rate 2: ");
			// Serial.print(int(flowRate2)); 
			// Serial.print("L/min");
			// Serial.print("\t");
        } else {
            // Serial.println("Failed to send Flow Rate Sensor 2 data to Firebase\n");
        }

		if (Firebase.setFloat(firebaseData, "/Sensors/totalMilliLitres2", totalMilliLitres2)) {
            // Serial.print("Output Liquid Quantity 2: ");
			// Serial.print(totalMilliLitres2);
			// Serial.print("mL / ");
			// Serial.print(totalMilliLitres2 / 1000);
			// Serial.println("L\n");
		} else {
			// Serial.println("Failed to send Total Mili Litres 2 data to Firebase\n");
		}
		// ==================== END OF SENSOR 2 ====================
	}

	// Day total volume from water flow 1
	if (Firebase.setFloat(firebaseData, "/Status/dayTotalVolume", totalMilliLitres1)) {
		// Serial.print("Day Total Volume: ");
		// Serial.print(totalMilliLitres1);
		// Serial.print("mL / ");
		// Serial.print(totalMilliLitres1 / 1000);
		// Serial.println("L\n");
	} else {
		// Serial.println("Failed to send Day Total Volume data to Firebase\n");
	}

	// Leak condition
	if (flowRate1 > (flowRate2 + (flowRate2 * 0.1))) {
		if (!leakDetected) {
            	Serial.println("\nLeak Detected!\n");
                leakDetectedTime = currentMillis1;
                recheckLeak = true;
                recheckTime = currentMillis1 + 5000; // Recheck after 5 seconds

				if (Firebase.setBool(firebaseData, "/Status/LeakDetected", true)) {
                    Serial.println("Leak Detected data sent to Firebase\n");
                } else {
                    Serial.println("Failed to send Leak Detected data to Firebase\n");
                }
		} else {
			if (currentMillis1 - leakDetectedTime >= 5000) {
				Serial.println("\n!!!!!!!!!! Leak Confirmed! !!!!!!!!!! \n");
				recheckLeak = false;

				if (Firebase.setBool(firebaseData, "/Status/LeakConfirmed", true)) {
					Serial.println("Leak Confirmed data sent to Firebase\n");
				} else {
					Serial.println("Failed to send Leak Confirmed data to Firebase\n");
				}
			} else if (recheckLeak && currentMillis1 >= recheckTime) {
				if (flowRate1 < (flowRate2 + (flowRate2 * 0.1))) {
					Serial.println("\nLeak Resolved\n");
					recheckLeak = false;
				}
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
		leakDetected = true;
	} else {
		if (leakDetected) {
			Serial.println("\nLeak Resolved\n");
			leakDetected = false;
			recheckLeak = false;
		}
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