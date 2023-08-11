#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FirebaseESP32.h>
#include <time.h>

#define LED_BUILTIN 2
#define SENSOR1 26
#define SENSOR2 27

long currentMillis = 0;
long previousMillis = 0;

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

unsigned long previousDay = 0;

unsigned int totalMilliLitres1Today = 0;

unsigned long pumpStartTime = 0;
unsigned long pumpRunningTime = 0;
boolean pumpRunning = false;

void IRAM_ATTR pulseCounter1()
{
	pulseCount1++;
}

void IRAM_ATTR pulseCounter2()
{
	pulseCount2++;
}

#define FIREBASE_HOST "your_firebase_host"
#define FIREBASE_AUTH "your_firebase_auth"

const char* ntpServer = "0.id.pool.ntp.org";
// GMT +7 25200, 7 * 60 * 60
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;

struct tm timeinfo;
FirebaseData firebaseData;
FirebaseJson json;

void connectToWiFi() {
    const char * ssid = "your_wifi_ssid";
    const char * password = "your_wifi_password";

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

void runningTimeLog(int Params, int totalMilliLitres1Today)
{
	if (!getLocalTime(&timeinfo)) {
		Serial.println("Failed to obtain time");
		return;
	}

	char timeYear[30];
	char timeMonth[30];
	char timeDay[30];
	strftime(timeYear, 30, "%Y", &timeinfo);
	strftime(timeMonth, 30, "%B", &timeinfo);
	strftime(timeDay, 30, "%d", &timeinfo);

	char pathNode[100];
	sprintf(pathNode, "pumpRunningHistory/%s/%s/%s", timeYear, timeMonth, timeDay);

	FirebaseJson updates;
	updates.set("runningTime", Params);
	updates.set("totalMilliLitres", totalMilliLitres1Today);

	Firebase.updateNodeSilentAsync(firebaseData, pathNode, updates);
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

	previousMillis = 0;

	Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);

	attachInterrupt(digitalPinToInterrupt(SENSOR1), pulseCounter1, FALLING);
	attachInterrupt(digitalPinToInterrupt(SENSOR2), pulseCounter2, FALLING);
}

void loop()
{
	if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        connectToWiFi();
    }

	currentMillis = millis();

	if (currentMillis - previousMillis > interval) {
		
		pulse1Sec1 = pulseCount1;
		pulse1Sec2 = pulseCount2;

		pulseCount1 = 0;
		pulseCount2 = 0;

		flowRate1 = ((1000.0 / (currentMillis - previousMillis)) * pulse1Sec1) / calibrationFactor;
		flowRate2 = ((1000.0 / (currentMillis - previousMillis)) * pulse1Sec2) / calibrationFactor;

		previousMillis = millis();

		flowMilliLitres1 = (flowRate1 / 60) * 1000;
		flowMilliLitres2 = (flowRate2 / 60) * 1000;

		totalMilliLitres1 += flowMilliLitres1;
		totalMilliLitres2 += flowMilliLitres2;

        json.set("Sensors/FlowRate1", flowRate1);
        json.set("Sensors/totalMilliLitres1", totalMilliLitres1);
        json.set("Sensors/pulseCount1", pulse1Sec1);
        json.set("Sensors/FlowRate2", flowRate2);
        json.set("Sensors/totalMilliLitres2", totalMilliLitres2);
        json.set("Sensors/pulseCount2", pulse1Sec2);
		json.set("Status/isRunning", pulse1Sec1 > 0);
		
		if (flowRate1 > (flowRate2 + (flowRate2 * 0.1))) {
			if (!leakDetected) {
				leakDetectedTime = currentMillis;
				recheckLeak = true;
				recheckTime = currentMillis + 5000;

				json.set("Status/LeakDetected", true);
				json.set("Status/LeakConfirmed", false);	
			} else {
				if (currentMillis - leakDetectedTime >= 5000) {
					recheckLeak = false;

					json.set("Status/LeakConfirmed", true);	
					json.set("Status/LeakDetected", true);		
				} else if (recheckLeak && currentMillis >= recheckTime) {
					if (flowRate1 < (flowRate2 + (flowRate2 * 0.1))) {
						recheckLeak = false;
					}
					json.set("Status/LeakDetected", false);
                    json.set("Status/LeakConfirmed", false);				
				}
			}
			leakDetected = true;
		} else {
			if (leakDetected) {
				leakDetected = false;
				recheckLeak = false;
			}
			json.set("Status/LeakDetected", false);
			json.set("Status/LeakConfirmed", false); 		
		}

		char timeDay[30];
		unsigned long currentDay = strftime(timeDay, 30, "%d", &timeinfo);

		if (currentDay != previousDay) {
			pumpRunningTime = 0;
			totalMilliLitres1Today = 0;
			previousDay = currentDay;
		}

		totalMilliLitres1Today += flowMilliLitres1;

		if (pulse1Sec1 > 0) {
			if (!pumpRunning) {
				pumpRunning = true;
				pumpStartTime = currentMillis;
			}
		} else {
			if (pumpRunning) {
				pumpRunning = false;
				pumpRunningTime += currentMillis - pumpStartTime;
			}
		}
		
		unsigned long totalRunningSeconds = pumpRunningTime / 1000;
		runningTimeLog(totalRunningSeconds, totalMilliLitres1Today);

		String path = "/";
		Firebase.updateNodeSilentAsync(firebaseData, path.c_str(), json);

		configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	}
}