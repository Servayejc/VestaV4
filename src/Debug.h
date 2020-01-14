//2020-01-12
#define LOGDATA_ON_SD
//#define LOGDATA_ON_AWS
//#define SEND_MAIL

#define DEBUG_
#ifdef DEBUG_
	#define DEBUG_MAIN
//	#define DEBUG_ESP_REMOTE
//  #define DEBUG_SETUP
//	#define DEBUG_NTP_TIME
//	#define DEBUG_HYSTORY
//	#define DEBUG_ELAPSED_TIME
	#define DEBUG_MEM
//	#define DEBUG_READ_CONFIG
//	#define DEBUG_SET_REMOTE_CONFIG
//	#define DEBUG_TEMPERATURES	
//	#define DEBUG_GET_REMOTE_VALUES
	#define DEBUG_SERVER
	#define DEBUG_UPDATE_DAY
	#define DEBUG_UPDATE_SETPOINTS
//	#define DEBUG_CONTROL
//	#define DEBUG_TEST_CONTROL
//	#define DEBUG_RTC
//	#define DEBUG_MDNS
//	#define DEBUG_BATTERY
// #define DEBUG_MQTT_SEND_MESSAGE
#endif


