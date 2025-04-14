#ifndef CONFIG_H
#define CONFIG_H

// User configuration variables
#define SYNC_INTERVAL 5000       // seconds
#define GPS_INTERVAL 5000        // seconds

#define FORMAT_24_HOUR false 

#define CLOCK_OFFSET 2000 // milliseconds
#define ASK_FREQUENCY 3300000 // Default: 55 minutes in milliseconds 55 * 60 * 1000
#define FAILED_SYNC_COUNT 3       // Default: 3 failed syncs
#define GEO_LOCATION_API "http://www.ip-api.com/line/?fields=offset"
#define NTP_SERVER "pool.ntp.org"


#endif // CONFIG_H

