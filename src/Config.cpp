//2020-01-12
#include <Arduino.h>
#include "SD.h"
#include "Config.h"
#include <ArduinoJson.h>
#include "Common.h"
#include "Utils.h"
#include "Debug.h"

char* wifi_SSID;
char* wifi_password;

char* _Mail_User;
char* _Mail_Pass;   		
char* _Mail_From;   	
char* _Mail_Dest;

// NoIP                   
char* _NOIP_Client;        
char* _NOIP_User;			
char* _NOIP_Pass;

#define  DEBUG_READ_CONFIG

void CopyItem(char*& s, char* v, JsonObject& root) {		// tested
	const char* temp = root[v].as<char*>(); 
	byte size = strlen(temp);
	s = (char*)malloc(size+1);
	strncpy(s,temp, size);
	s[size] = 0;
}  

byte hex_char_to_binary(char ch) {							// tested
	if(isdigit(ch))
	return ch - '0';             
	else 
	return ( ch - 'A' ) + 10; 
}

void hex_to_byte (const char* str, byte ndx) {				// tested
	const char* ptr = str;
	byte high, low; 
	for(int i=0; i < 8; i++) {
		high = hex_char_to_binary(*ptr); 
		low = hex_char_to_binary(*(ptr+1));
		//Sensors[ndx][i] = low | ( high << 4 );
		// NEW code 
		Sensors[ndx].Address[i] = low | ( high << 4 );  
		//
		#ifdef DEBUG_READ_CONFIG
		    //Serial.print(Sensor[ndx][i], HEX);
			//Serial.print(" ");
			//Serial.print(Sensors[ndx].Address[i], HEX);
		    //Serial.print(" ");
		#endif	
		ptr++;
		ptr++;	
	}
	#ifdef	 DEBUG_READ_CONFIG
	//Serial.println();
	#endif
}

void readSensorsConfig() {									// tested
	Sensors.clear();
	
	//#ifdef DEBUG_READ_CONFIG
	Serial.println();
	Serial.println(F("Read Sensors Config"));
	//#endif
	File configFile;
	//configFile = SD.open("Sensors.jsn");
	configFile = SPIFFS.open("/Sensors.jsn", "r");
	if (configFile) {
		StaticJsonBuffer<2000> jsonBuffer;
		// Parse the root object
		JsonObject &root = jsonBuffer.parseObject(configFile);
		if (root.success()) {
			#ifdef DEBUG_READ_CONFIG
				Serial.println("Root Success");
			#endif	
			for(int s = 0; s < (int)root["Sensors"].size(); s++) {
				// NEW code
				Sensors.push_back(SensorEntry());
				Sensors[s].Ndx = root["Sensors"][s]["N"];
				Sensors[s].Port = root["Sensors"][s]["P"];
				if (Sensors[s].Port == 0) {  
					Sensors[s].Ready = true;	// master 
				} else {
					Sensors[s].Ready = false;	// remote
					// add station id needed
					
					
				}
				Sensors[s].Channel = root["Sensors"][s]["C"];
				Sensors[s].TT = 0;
				Sensors[s].NT = 0;
				
				const char* temp = root["Sensors"][s]["DSC"].as <char*>(); 
				byte size = strlen(temp);
				if (size > 15) {
					size = 15;
				}
				strncpy(Sensors[s].Desc,temp, size);
				Sensors[s].Desc[size] = 0;

				
				//strncpy( Sensors[s].Desc,root["Sensors"][s]["DSC"].as <char*>(),15);
				hex_to_byte(root["Sensors"][s]["A"].as<char*>(), s);
				Sensors[s].SP[0] = 0;
				#ifdef DEBUG_READ_CONFIG
					printSensor(s);
				#endif
			}
			#ifdef DEBUG_READ_CONFIG
			Serial.println(F("Sensors initialized"));
			#endif 
		}
		configFile.close();
		#ifdef DEBUG_READ_CONFIG
		Serial.println("Sensors.jsn closed ");
		#endif	
	}
	else
	{
		Serial.print("file error : ");
		Serial.println("Sensors.jsn");
	}	
}

void readStationsConfig() {									// tested
	#ifdef DEBUG_READ_CONFIG
	Serial.println();
	Serial.println(F("Read config"));
	#endif
	File configFile;
	configFile = SPIFFS.open("/Config.jsn", "r");
//	configFile = SD.open("Config.jsn");
	if (configFile) {
		#ifdef DEBUG_READ_CONFIG
		Serial.println(F("Config file Opened"));
		#endif			
		
		StaticJsonBuffer<2000> jsonBuffer;
		// Parse the root object
		JsonObject &root = jsonBuffer.parseObject(configFile);
		if (root.success()) {
			// save config strings to global variables because
			// the json object will be deleted at the end of this function	
			CopyItem(ntp_URL, "NTP", root);
			CopyItem(wifi_SSID,"SSID", root);
			CopyItem(wifi_password, "Password", root);
			

			CopyItem(_Mail_User, "User", root["MAIL"]);
			CopyItem(_Mail_Pass, "Pass", root["MAIL"]);
			CopyItem(_Mail_From, "From", root["MAIL"]);
			CopyItem(_Mail_Dest, "Dest", root["MAIL"]);
			
			CopyItem(_NOIP_Client, "Client", root["NOIP"]);
			CopyItem(_NOIP_User, "User", root["NOIP"]);
			CopyItem(_NOIP_Pass, "Pass", root["NOIP"]);
			#ifdef DEBUG_READ_CONFIG
			Serial.println(F("Credentials initialized"));
			#endif 
		}
		configFile.close();
		#ifdef DEBUG_READ_CONFIG
		Serial.println("Config.jsn closed ");	
		#endif		
	}
	else
	{
		#ifdef DEBUG_READ_CONFIG
		Serial.print("file error : ");
		Serial.println("Config.jsn");
		#endif	
	}	
	#ifdef DEBUG_READ_CONFIG
		Serial.print(F("After read config : "));
		checkMem();
		// TODO add Stations info
		
		Serial.println();
		Serial.println(ntp_URL);
		Serial.println(wifi_SSID);
		Serial.println(wifi_password);
		Serial.println(_Mail_User);
		Serial.println(_Mail_Pass);
		Serial.println(_Mail_From);
		Serial.println(_Mail_Dest);
		Serial.println(_NOIP_Client);
		Serial.println(_NOIP_User);
		Serial.println(_NOIP_Pass);
		Serial.println();	
	#endif
}
		
