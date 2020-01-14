//fdfff
#ifndef COMMON_H_
#define COMMON_H_

#include <Common.h>
#include <vector>            
#include <Arduino.h>
#include <Ethernet.h>

#define CONTROL  0x38
#define DISPLAY_ADD  0x3A 


//Regulator
extern float SetPoints[6];	// default values, set by wifi 
extern float Hysteresis;	
extern byte Output;
extern bool Auto;

// NTP Server
extern char* ntp_URL;

// WIFI
extern char* wifi_SSID;
extern char* wifi_password;

// Mail 
extern char* _Mail_User;
extern char* _Mail_Pass;   		
extern char* _Mail_From;   	
extern char* _Mail_Dest;

// NoIP                   
extern char* _NOIP_Client;        
extern char* _NOIP_User;			
extern char* _NOIP_Pass;

// NEW code
struct SensorEntry {
	int Ndx;
	char Desc[16];
	uint8_t Address[8] ;   
	uint8_t Port;	
	uint8_t Channel;
	bool Ready;
	int Time[9] ;	//	H*60 + M
	int SP[9];		//  SP * 100 	
	float TT;
	int NT;
	int NdxSP;
//	time_t LastUpdate;
	bool fault; 
	bool Ala;
	float SOC;
	float Bat;
 };

typedef std::vector<SensorEntry> SensorsList;
extern SensorsList Sensors;

struct StationEntry {
	int Port;
	IPAddress IP;
	String URL;
	bool Ready;
};

typedef std::vector<StationEntry> StationsList;
extern StationsList Stations;

struct ChannelEntry {
	int SensorNdx;
	int Time[9] ;	//	H*60 + M
	int SP[9];		//  SP * 100
};

/*typedef std::vector<AlarmEntry> AlarmsList;
extern AlarmsList Alarms;

struct AlarmEntry {
	int ID;
	time_t Time;
	int Status;
};*/

typedef std::vector<ChannelEntry> ChannelList;
extern ChannelList Channels;
extern bool SensorsOk;
extern int LastMin;
extern bool DayChanged;
extern bool TempReaded;
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;
extern long connection;
extern int arrivedcount;
extern bool Sended;
extern bool ConfigChanged;

#endif
