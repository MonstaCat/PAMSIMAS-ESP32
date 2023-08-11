#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FirebaseESP32.h>
#include <time.h>
#include <HTTPClient.h>

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
String statusMessage;

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

const char* ntpServer = "0.id.pool.ntp.org";
// GMT +7 25200, 7 * 60 * 60
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;

const char* serverUrl = "http://your_local_server/pamsimas_data_logging/log_data.php";

struct tm timeinfo;

void connectToWiFi() {
    const char * ssid = "_blank";
    const char * password = "_bl4nk_123";

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

String getFormattedTime()
{
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return ""; // Return an empty string in case of failure
    }

    char timeHour[3];
    char timeMinute[3];
    char timeSecond[3];
    snprintf(timeHour, 3, "%02d", timeinfo.tm_hour);
    snprintf(timeMinute, 3, "%02d", timeinfo.tm_min);
    snprintf(timeSecond, 3, "%02d", timeinfo.tm_sec);

    String formattedTime = String(timeHour) + ":" + String(timeMinute) + ":" + String(timeSecond);
    return formattedTime;
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

		if (flowRate1 > (flowRate2 + (flowRate2 * 0.1))) {
			if (!leakDetected) {
				leakDetectedTime = currentMillis;
				recheckLeak = true;
				recheckTime = currentMillis + 5000;

				statusMessage = "Leak Detected";
			}
			else
			{
				if (currentMillis - leakDetectedTime >= 5000) {
					recheckLeak = false;

					statusMessage = "!!!!!!!!!!!! Leak Confirmed !!!!!!!!!!!!";
				} else if (recheckLeak && currentMillis >= recheckTime) {
					if (flowRate1 < (flowRate2 + (flowRate2 * 0.1))) {
						recheckLeak = false;
					}
					statusMessage = "Leak Resolved";
				}
			}
			leakDetected = true;
		} else {
			if (leakDetected) {
				leakDetected = false;
				recheckLeak = false;
			}
			statusMessage = "Leak Resolved";
		}

		String currentTime = getFormattedTime();

		String data = "time=" + currentTime +
						"&flowRate1=" + String(flowRate1) +
						"&flowRate2=" + String(flowRate2) +
						"&totalMilliLitres1=" + String(totalMilliLitres1) +
						"&totalMilliLitres2=" + String(totalMilliLitres2) +
						"&status=" + statusMessage;
						
		HTTPClient http;
		http.begin(serverUrl);
		http.addHeader("Content-Type", "application/x-www-form-urlencoded");
		int httpResponseCode = http.POST(data);
		if (httpResponseCode == HTTP_CODE_OK) {
			Serial.println("Data logged successfully!");
		} else {
			Serial.println("Failed to log data. Error code: " + String(httpResponseCode));
		}
		http.end();

		configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	}
}