#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FirebaseESP32.h>

#define LED_BUILTIN 2

#define SENSOR1 26
#define SENSOR2 27

long currentMillis1 = 0;
long previousMillis1 = 0;

long currentMillis2 = 0;
long previousMillis2 = 0;

int interval = 1000;
float calibrationFactor = 1.6;

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

#define FIREBASE_HOST "https://pamsimas-firebase-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyAzyAYysQNeR4LtxCo3OmpK-WfJ3XHxnY0"

FirebaseData firebaseData;

void connectToWiFi() {
    const char * ssid = "Revaille12";
    const char * password = "revaille12";

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

	pinMode(LED_BUILTIN, OUTPUT);

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
	if (WiFi.status() == WL_CONNECTED) {
		digitalWrite(LED_BUILTIN, HIGH);
		delay(1000);
		digitalWrite(LED_BUILTIN, LOW); 
		delay(1000);
	}

	if (pulseCount1 > 0) {
		Firebase.setBoolAsync(firebaseData, "/Status/isRunning", true);
	} else {
		Firebase.setBoolAsync(firebaseData, "/Status/isRunning", false);
	}

	currentMillis1 = millis();
	currentMillis2 = millis();

	if (currentMillis1 - previousMillis1 > interval) {
		
		pulse1Sec1 = pulseCount1;
		pulseCount1 = 0;

		flowRate1 = ((1000.0 / (millis() - previousMillis1)) * pulse1Sec1) / calibrationFactor;

		previousMillis1 = millis();

		flowMilliLitres1 = (flowRate1 / 60) * 1000;

		totalMilliLitres1 += flowMilliLitres1;

		Firebase.setFloatAsync(firebaseData, "/Sensors/FlowRate1", flowRate1);
		Firebase.setIntAsync(firebaseData, "/Sensors/totalMilliLitres1", totalMilliLitres1);
		Firebase.setFloatAsync(firebaseData, "/Sensors/pulseCount1", pulse1Sec1);
	}

	if (currentMillis2 - previousMillis2 > interval) {
		
		pulse1Sec2 = pulseCount2;
		pulseCount2 = 0;

		flowRate2 = ((1000.0 / (millis() - previousMillis2)) * pulse1Sec2) / calibrationFactor;

		previousMillis2 = millis();

		flowMilliLitres2 = (flowRate2 / 60) * 1000;

		totalMilliLitres2 += flowMilliLitres2;

        Firebase.setFloatAsync(firebaseData, "/Sensors/FlowRate2", flowRate2);
		Firebase.setIntAsync(firebaseData, "/Sensors/totalMilliLitres2", totalMilliLitres2);
		Firebase.setFloatAsync(firebaseData, "/Sensors/pulseCount2", pulse1Sec2);
	}

	Firebase.setIntAsync(firebaseData, "/Status/dayTotalVolume", totalMilliLitres1);

	if (flowRate1 > (flowRate2 + (flowRate2 * 0.1))) {
		if (!leakDetected) {
			leakDetectedTime = currentMillis1;
			recheckLeak = true;
			recheckTime = currentMillis1 + 5000;

			Firebase.setBool(firebaseData, "/Status/LeakDetected", true); 
		} else {
			if (currentMillis1 - leakDetectedTime >= 5000) {
				recheckLeak = false;

				Firebase.setBool(firebaseData, "/Status/LeakConfirmed", true);
			} else if (recheckLeak && currentMillis1 >= recheckTime) {
				if (flowRate1 < (flowRate2 + (flowRate2 * 0.1))) {
					recheckLeak = false;
				}
				Firebase.setBool(firebaseData, "/Status/LeakConfirmed", false); 
				Firebase.setBool(firebaseData, "/Status/LeakDetected", false); 
			}
		}
		leakDetected = true;
	} else {
		if (leakDetected) {
			leakDetected = false;
			recheckLeak = false;
		}
		Firebase.setBool(firebaseData, "/Status/LeakConfirmed", false); 
		Firebase.setBool(firebaseData, "/Status/LeakDetected", false);
	}
}