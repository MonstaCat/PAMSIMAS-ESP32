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

	previousMillis1 = 0;
	previousMillis2 = 0;

	attachInterrupt(digitalPinToInterrupt(SENSOR1), pulseCounter1, FALLING);
	attachInterrupt(digitalPinToInterrupt(SENSOR2), pulseCounter2, FALLING);
}

void loop()
{
	currentMillis1 = millis();
	currentMillis2 = millis();

	if (currentMillis1 - previousMillis1 > interval) {
		
		pulse1Sec1 = pulseCount1;
		pulseCount1 = 0;

		flowRate1 = ((1000.0 / (millis() - previousMillis1)) * pulse1Sec1) / calibrationFactor;

		previousMillis1 = millis();

		flowMilliLitres1 = (flowRate1 / 60) * 1000;

		totalMilliLitres1 += flowMilliLitres1;

        Serial.print("Flow rate: ");
        Serial.print(int(flowRate1));
        Serial.print("L/min");
        Serial.print("\t");

        Serial.print("Output Liquid Quantity: ");
        Serial.print(totalMilliLitres1);
        Serial.print("mL / ");
        Serial.print(totalMilliLitres1 / 1000);
        Serial.println("L");
	}

	if (currentMillis2 - previousMillis2 > interval) {
		
		pulse1Sec2 = pulseCount2;
		pulseCount2 = 0;

		flowRate2 = ((1000.0 / (millis() - previousMillis2)) * pulse1Sec2) / calibrationFactor;

		previousMillis2 = millis();

		flowMilliLitres2 = (flowRate2 / 60) * 1000;

		totalMilliLitres2 += flowMilliLitres2;

        Serial.print("Flow rate: ");
        Serial.print(int(flowRate1));
        Serial.print("L/min");
        Serial.print("\t");

        Serial.print("Output Liquid Quantity: ");
        Serial.print(totalMilliLitres1);
        Serial.print("mL / ");
        Serial.print(totalMilliLitres1 / 1000);
        Serial.println("L");	
    }

	if (flowRate1 > (flowRate2 + (flowRate2 * 0.1))) {
		if (!leakDetected) {
			leakDetectedTime = currentMillis1;
			recheckLeak = true;
			recheckTime = currentMillis1 + 5000;

            Serial.println("\nLeak Detected\n");
        }
        else
        {
            if (currentMillis1 - leakDetectedTime >= 5000) {
				recheckLeak = false;

                Serial.println("\n!!!!!!!!!!!! Leak Confirmed !!!!!!!!!!!!\n");
			} else if (recheckLeak && currentMillis1 >= recheckTime) {
				if (flowRate1 < (flowRate2 + (flowRate2 * 0.1))) {
					recheckLeak = false;
				}
                Serial.println("\nLeak Resolved\n");
			}
        }
        leakDetected = true;
	} else {
		if (leakDetected) {
			leakDetected = false;
			recheckLeak = false;
		}
        Serial.println("\nLeak Resolved\n");
	}
}