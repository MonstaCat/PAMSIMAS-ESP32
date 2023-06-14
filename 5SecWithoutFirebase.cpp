#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FirebaseESP32.h>

#define SENSOR1 26
#define SENSOR2 27

long currentMillis = 0;
long previousMillis = 0;

int interval = 5000;
float calibrationFactor = 1.32;

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

void setup()
{
	Serial.begin(115200);

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

        Serial.print("Flow rate 1: ");
        Serial.print(int(flowRate1));
        Serial.print("L/min");
        Serial.print("\t");

        Serial.print("Output Liquid Quantity 1: ");
        Serial.print(totalMilliLitres1);
        Serial.print("mL / ");
        Serial.print(totalMilliLitres1 / 1000);
        Serial.println("L");

		Serial.print("Flow rate 2: ");
        Serial.print(int(flowRate1));
        Serial.print("L/min");
        Serial.print("\t");

        Serial.print("Output Liquid Quantity 2: ");
        Serial.print(totalMilliLitres2);
        Serial.print("mL / ");
        Serial.print(totalMilliLitres2 / 1000);
        Serial.println("L\n");	

		if (flowRate1 > (flowRate2 + (flowRate2 * 0.1))) {
			if (!leakDetected) {
				leakDetectedTime = currentMillis;
				recheckLeak = true;
				recheckTime = currentMillis + 5000;

				Serial.println("Leak Detected\n");
			}
			else
			{
				if (currentMillis - leakDetectedTime >= 5000) {
					recheckLeak = false;

					Serial.println("!!!!!!!!!!!! Leak Confirmed !!!!!!!!!!!!\n");
				} else if (recheckLeak && currentMillis >= recheckTime) {
					if (flowRate1 < (flowRate2 + (flowRate2 * 0.1))) {
						recheckLeak = false;
					}
					Serial.println("Leak Resolved\n");
				}
			}
			leakDetected = true;
		} else {
			if (leakDetected) {
				leakDetected = false;
				recheckLeak = false;
			}
			Serial.println("Leak Resolved\n");
		}
	}
}