#ifndef TimeClient_H
#define TimeClient_H
/*

 WTA Client

 Get the time from the worldtimeapi-server to prevent timezone and DST-mess
 Demonstrates use http-client and json-parser

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 updated for the ESP8266 12 Apr 2015 
 by Ivan Grokhotkov
 Refactored into NTPClient class by Hari Wiguna, 2018
 changed from NTP to worldtimeapi by SnowHead, 2019

 This code is in the public domain.

 */
#include <Arduino.h>
#include <ESP8266WiFi.h>

extern unsigned long askFrequency;

class TimeClient 
{
public:
  TimeClient();
  void Setup(void);
  unsigned long GetCurrentTime();
  byte GetHours();
  byte GetMinutes();
  byte GetSeconds();
  void PrintTime();
  bool SaveConfig(void);
  bool LoadConfig(void);

  // Get the number of successive failed syncs
  unsigned int GetFailedSyncCount() const { return failedSyncCount; }

  // New setter for the timezone offset from the API
  void setTimeZoneOffset(long offset);

  // New setter for the millisecond offset (from CLOCK_OFFSET in config.h)
  void setMsOffset(int msOffset);

private:
  void AskCurrentEpoch();
  unsigned long ReadCurrentEpoch();

  unsigned int failedSyncCount = 0; // Tracks the number of successive failed syncs

  // Store the dynamic offsets in TimeClient:
  long _timeZoneOffset = 0; // in seconds
  int _msOffset = 0;        // in milliseconds

  // Cached time values
  unsigned long lastEpoch = 0;
  unsigned long lastEpochTimeStamp = 0;
  unsigned long currentTime = 0;
};

#endif // TimeClient_H