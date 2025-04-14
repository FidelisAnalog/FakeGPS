// Refactor v1
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <TimeClient.h>			  // https://github.com/arduino-libraries/NTPClient
#include <ESP8266WiFi.h>
#include "config.h" // Include the configuration header

//== DOUBLE-RESET DETECTOR ==
#include <DoubleResetDetector.h>	// https://github.com/datacute/DoubleResetDetector
#define DRD_TIMEOUT 2 // Second-reset must happen within 2 seconds of first reset to be considered a double-reset
#define DRD_ADDRESS 0 // RTC Memory Address for the DoubleResetDetector to use
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

TimeClient timeClient;
WiFiManager wifiManager;
time_t netEpoch = 0;

//int firstrun = 1;

enum LEDMode
{
    LED_OFF,
    LED_ON,
    LED_FAST_BLINK
};

void handleLED(LEDMode mode);

void configPortalStarted(WiFiManager *wm)
{
    handleLED(LED_ON); // Turn on the LED (active-low)
}

void syncTime();
void generateGPSSentence();

unsigned long lastSyncTime = 0;
unsigned long lastGPSTime = 0;

// Configuration variable for the last sync threshold (in seconds)
unsigned long lastSyncThreshold = 30; // Default: 1 hour

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);   // Initialize the LED_BUILTIN pin as an output
    handleLED(LED_OFF);             // Turn off the LED initially (active-low)
    Serial.begin(115200);
    Serial.print("\n\n\n");
    Serial.println("FakeGPS Starting up...");

    wifiManager.setTimeout(120);

    // Set the callback for when the config portal starts
    wifiManager.setAPCallback(configPortalStarted);

    if (drd.detectDoubleReset())
    {
        Serial.println("DOUBLE Reset Detected");
        handleLED(LED_ON); // Turn on the LED (active-low)
        WiFi.disconnect();
        wifiManager.startConfigPortal("FakeGPS");
    }
    else
    {
        Serial.println("SINGLE reset Detected");
        // Attempt to auto-connect
        if (!wifiManager.autoConnect("FakeGPS"))
        {
            Serial.println("Failed to connect and hit timeout");
            delay(3000);
            ESP.reset(); // Reset and try again
            delay(5000);
        }
    }

    drd.stop();

    handleLED(LED_OFF); // Turn off the LED after successful connection

    Serial.print("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("... Connected\n");

    Serial.println(String("WiFi BSSID: ") + WiFi.BSSIDstr() + 
                   String(", Channel: ") + WiFi.channel() + 
                   String(", RSSI: ") + WiFi.RSSI());

    Serial.println(String("STA IP: ") + WiFi.localIP().toString() + 
                   String(", Netmask: ") + WiFi.subnetMask().toString() + 
                   String(", Gateway: ") + WiFi.gatewayIP().toString() + 
                   String(", DNS1: ") + WiFi.dnsIP(0).toString() + 
                   String(", DNS2: ") + WiFi.dnsIP(1).toString());

    Serial1.begin(9600);

    wifiManager.setCustomHeadElement("<style>form[action='/exit'], form[action='/erase'] { display: none; }</style>");
    wifiManager.startWebPortal();
}

void loop()
{
    // Periodically check for time synchronization
    if (millis() - lastSyncTime >= SYNC_INTERVAL)
    {
        lastSyncTime = millis();
        syncTime();
    }

    // Periodically generate GPS sentences
    if (millis() - lastGPSTime >= GPS_INTERVAL)
    {
        lastGPSTime = millis();
        generateGPSSentence();
    }

    // Add other tasks here
    wifiManager.process();

    //handleLED(LED_FAST_BLINK);

}

void syncTime()
{
    // Call GetCurrentTime to ensure the time is updated if needed
    time_t updatedTime = timeClient.GetCurrentTime();

    // Check if the last sync was successful
    if (timeClient.GetFailedSyncCount() == 0)
    {
        netEpoch = updatedTime;
        //handleLED(LED_WINK); // Wink to indicate a successful sync
        //Serial.println("NTP synchronization successful.");
    }
    else
    {
        //Serial.println("NTP synchronization failed.");

        // Check if the number of failed syncs exceeds the threshold
        if (timeClient.GetFailedSyncCount() >= FAILED_SYNC_COUNT)
        {
            handleLED(LED_FAST_BLINK); // Fast blink to indicate repeated sync failures
            Serial.println(String("NTP has failed ") + FAILED_SYNC_COUNT + String(" times"));
        }
    }
}

void generateGPSSentence()
{
    char tstr[128];
    unsigned char cs;
    unsigned int i;
    struct tm *tmtime;

    if (netEpoch)
    {
        tmtime = localtime(&netEpoch);

        // Generate GPS sentence
        //handleLED(LED_WINK); // Blink for sync
        sprintf(tstr, "$GPRMC,%02d%02d%02d,A,0000.0000,N,00000.0000,E,0.0,0.0,%02d%02d%02d,0.0,E,S",
                tmtime->tm_hour, tmtime->tm_min, tmtime->tm_sec, tmtime->tm_mday, tmtime->tm_mon + 1, tmtime->tm_year - 100);
        cs = 0;
        for (i = 1; i < strlen(tstr); i++) // Calculate checksum
            cs ^= tstr[i];
        sprintf(tstr + strlen(tstr), "*%02X", cs);
        Serial.println(tstr);  // Send to console
        Serial1.println(tstr); // Send to clock
        //handleLED(LED_OFF);
    }
}

void handleLED(LEDMode mode)
{
    static unsigned long lastBlinkTime = 0;
    static bool ledState = false;
    //static bool winkState = false;
    //static unsigned long winkStartTime = 0;

    unsigned long currentTime = millis();

    switch (mode)
    {
    case LED_OFF:
        digitalWrite(LED_BUILTIN, HIGH); // Turn off LED (active-low)
        break;

    case LED_ON:
        digitalWrite(LED_BUILTIN, LOW); // Turn on LED (active-low)
        break;

    case LED_FAST_BLINK:
        if (currentTime - lastBlinkTime >= 100) // Fast blink every 100ms
        {
            lastBlinkTime = currentTime;
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
            //Serial.println("LED Fast Blink");
        }
        break;
    /*
    case LED_SLOW_BLINK:
        if (currentTime - lastBlinkTime >= 1000) // Slow blink every 500ms
        {
            lastBlinkTime = currentTime;
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
        }
        break;

    case LED_WINK:
        if (!winkState) // Start wink
        {
            winkState = true;
            winkStartTime = currentTime;
            digitalWrite(LED_BUILTIN, LOW); // Turn on LED (active-low)
        }
        else if (currentTime - winkStartTime >= 200) // Wink duration: 200ms
        {
            winkState = false;
            digitalWrite(LED_BUILTIN, HIGH); // Turn off LED (active-low)
        }
        break;
        */
    }
}

