// Refactor v1
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <TimeClient.h>			  // https://github.com/arduino-libraries/NTPClient

//== DOUBLE-RESET DETECTOR ==
#include <DoubleResetDetector.h>	// https://github.com/datacute/DoubleResetDetector
#define DRD_TIMEOUT 2 // Second-reset must happen within 2 seconds of first reset to be considered a double-reset
#define DRD_ADDRESS 0 // RTC Memory Address for the DoubleResetDetector to use
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

TimeClient timeClient;
WiFiManager wifiManager;
time_t locEpoch = 0, netEpoch = 0;

int firstrun = 1;

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);   // Initialize the LED_BUILTIN pin as an output
	digitalWrite(LED_BUILTIN, HIGH);
	Serial.begin(115200);
	Serial.print("\n\n\n");
	Serial.println("FakeGPS Starting up...");

	wifiManager.setTimeout(180);

	if (drd.detectDoubleReset())
	{
		Serial.println("DOUBLE Reset Detected");
		digitalWrite(LED_BUILTIN, LOW);
		WiFi.disconnect();
		wifiManager.startConfigPortal("FakeGPS");
	}
	else
	{
		Serial.println("SINGLE reset Detected");
		digitalWrite(LED_BUILTIN, HIGH);
		//fetches ssid and pass from eeprom and tries to connect
		//if it does not connect it starts an access point with the specified name wifiManagerAPName
		//and goes into a blocking loop awaiting configuration
		if (!wifiManager.autoConnect("FakeGPS"))
		{
			Serial.println("Failed to connect and hit timeout");
			delay(3000);
			//reset and try again, or maybe put it to deep sleep
			ESP.reset();
			delay(5000);
		}
	}

	drd.stop();

	digitalWrite(LED_BUILTIN, HIGH);

	Serial.print("Waiting for WiFi");
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.print("... Connected\n");

	Serial.print("STA IP address: ");
	Serial.println(WiFi.localIP());

	Serial1.begin(9600);
}


void loop()
{
	char tstr[128];
	unsigned char cs;
	unsigned int i;
	struct tm *tmtime;
	unsigned long amicros, umicros = 0;

	for (;;)
	{
		amicros = micros();
		while (((netEpoch = timeClient.GetCurrentTime()) == locEpoch) || (!netEpoch))
		{
			delay(100);
		}
		if (netEpoch)
		{
			umicros = amicros;
			tmtime = localtime(&netEpoch);

			if ((!tmtime->tm_sec) || firstrun)			// full minute or first cycle
			{
				digitalWrite(LED_BUILTIN, HIGH);		// blink for sync
				sprintf(tstr, "$GPRMC,%02d%02d%02d,A,0000.0000,N,00000.0000,E,0.0,0.0,%02d%02d%02d,0.0,E,S",
						tmtime->tm_hour, tmtime->tm_min, tmtime->tm_sec, tmtime->tm_mday, tmtime->tm_mon + 1, tmtime->tm_year - 100);
				cs = 0;
				for (i = 1; i < strlen(tstr); i++)		// calculate checksum
					cs ^= tstr[i];
				sprintf(tstr + strlen(tstr), "*%02X", cs);
				Serial.println(tstr);					// send to console
				Serial1.println(tstr);					// send to clock
				delay(100);
				digitalWrite(LED_BUILTIN, LOW);
				if(tmtime->tm_sec < 57)
				{
					delay(58000 - ((micros() - amicros) / 1000) - (tmtime->tm_sec * 1000)); // wait for end of minute
				}
				firstrun = 0;
			}
		}
		delay(200);
		if (((amicros - umicros) / 1000000L) > 3600)	// if no sync for more than one hour
			digitalWrite(LED_BUILTIN, HIGH);			// switch off LED
	}
}

