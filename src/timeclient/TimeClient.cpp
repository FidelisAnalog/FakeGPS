#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
//#include <NTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h" // Include the configuration header

//=== NTP CLIENT ===
#include <TimeClient.h>

#define DEBUG 1

//const char *geo_location_api = "http://www.ip-api.com/line/?fields=offset";

HTTPClient http;
WiFiClient wifiClient;
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);
String payload;

unsigned long timeToAsk;
unsigned long timeToRead;
unsigned long lastEpoch; // We don't want to continually ask for epoch from time server, so this is the last epoch we received (could be up to an hour ago based on askFrequency)
unsigned long lastEpochTimeStamp; // What was millis() when asked server for Epoch we are currently using?
unsigned long nextEpochTimeStamp; // What was millis() when we asked server for the upcoming epoch
unsigned long currentTime;
struct tm local_tm;

//== PREFERENCES == (Fill these appropriately if you could not connect to the ESP via your phone)
//char homeWifiName[] = ""; // PREFERENCE: The name of the home WiFi access point that you normally connect to.
//char homeWifiPassword[] = ""; // PREFERENCE: The password to the home WiFi access point that you normally connect to.
//bool error_getTime = false;

unsigned long date_time = 0;

long _timeZoneOffset = 0;
int _msOffset = 0;

TimeClient::TimeClient()
{
}

void TimeClient::Setup(void)
{
}

void TimeClient::setTimeZoneOffset(long offset) {
    _timeZoneOffset = offset;
}

void TimeClient::setMsOffset(int msOffset) {
    _msOffset = msOffset;
}

void TimeClient::AskCurrentEpoch()
{
    int httpCode;
    int offset = 0;

    if (DEBUG)
    {
        Serial.println("Asking server for current epoch");
        Serial.print("Timezone offset (s): ");
    }

    // Call the API to get the offset
    http.begin(wifiClient, GEO_LOCATION_API);
    httpCode = http.GET();

    payload = "";
    if (httpCode > 0)
    {
        payload = http.getString();
    }
    http.end();

    if (DEBUG)
        Serial.println(payload.c_str());

    if (payload.length())
    {
        // Parse the offset value
        auto error = (sscanf(payload.c_str(), "%d", &offset) != 1);
        if (error)
        {
            if (DEBUG)
                Serial.println("Scan timezone offset failed");
            offset = 0;
        }
        else
        {
            // Store dynamic offset locally:
            setTimeZoneOffset(offset);
            // Set the millisecond offset from config using CLOCK_OFFSET:
            setMsOffset(CLOCK_OFFSET);

            // Update NTP time without trying to modify its private member:
            ntpClient.update();
            if ((lastEpoch = ntpClient.getEpochTime()))
            {
                failedSyncCount = 0;       // Reset failed sync count on success
                lastEpochTimeStamp = millis();
            }
            else
            {
                failedSyncCount++;
            }
            if (DEBUG)
            {
                Serial.print("Server epoch: ");
                Serial.println(lastEpoch);
            }
        }
    }
    else
    {
        failedSyncCount++;
    }
}

unsigned long TimeClient::ReadCurrentEpoch()
{
	lastEpoch = date_time;
	lastEpochTimeStamp = nextEpochTimeStamp;
	return lastEpoch;
}

unsigned long TimeClient::GetCurrentTime()
{
    unsigned long timeNow = millis();

    // Check if it's time to ask the server for the current time
    if (timeNow >= timeToAsk)
    {
        if (DEBUG)
            Serial.println("Time to ask server for current time");

        timeToAsk = timeNow + ASK_FREQUENCY; // Schedule the next time to ask
        timeToRead = timeNow + 1000;         // Wait 1 second for the server to respond
        AskCurrentEpoch();                  // Fetch the current epoch from the server
    }

    // Instead of reading with ReadCurrentEpoch that overwrites lastEpoch with stale values,
    // simply compute current time from the cached lastEpoch.
    /*
    if (timeToRead > 0 && timeNow >= timeToRead)
    {
        ReadCurrentEpoch(); // Remove or modify this call
        timeToRead = 0;
    }
    */

    // Calculate the current time based on the last known epoch and elapsed time
    if (lastEpoch != 0)
    {
        unsigned long elapsedMillis = millis() - lastEpochTimeStamp;
        currentTime = lastEpoch + (elapsedMillis / 1000) + _timeZoneOffset + (_msOffset / 1000);
    }

    return currentTime;
}

byte TimeClient::GetHours()
{
	return (currentTime % 86400L) / 3600;
}

byte TimeClient::GetMinutes()
{
	return (currentTime % 3600) / 60;
}

byte TimeClient::GetSeconds()
{
	return currentTime % 60;
}

void TimeClient::PrintTime()
{
#if (DEBUG)
	// print the hour, minute and second:
	Serial.print("The local time is: ");
	byte hh = GetHours();
	byte mm = GetMinutes();
	byte ss = GetSeconds();

	Serial.print(hh); // print the hour (86400 equals secs per day)
	Serial.print(':');
	if (mm < 10)
	{
		// In the first 10 minutes of each hour, we'll want a leading '0'
		Serial.print('0');
	}
	Serial.print(mm); // print the minute (3600 equals secs per minute)
	Serial.print(':');
	if (ss < 10)
	{
		// In the first 10 seconds of each minute, we'll want a leading '0'
		Serial.print('0');
	}
	Serial.println(ss); // print the second
#endif
}
