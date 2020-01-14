//2020-01-12
#include "Control.h" 
#include "Common.h"
#include "Utils.h"
#include "RTC.h"
#include <Wire.h>
#include <ArduinoJson.h>
#include "Debug.h"
#include <FS.h>
#include "SD.h"

void ControlTest() {
	Output = 0;
	for ( byte i = 0; i < 8; i++) {
		bitSet(Output, i);
		Wire.beginTransmission(CONTROL);
		Wire.write(Output);
		Wire.endTransmission();
		delay(200);
		bitClear(Output, i);
		delay(200);
		Wire.beginTransmission(CONTROL);
		Wire.write(Output);
		Wire.endTransmission();
	}	
}

void ControlTemp() {
	int y = 6;
	for ( byte i = 0; i < Sensors.size(); i++) {
		int channel = Sensors[i].Channel - 1;
		if (channel > -1)  {
			int NT = Sensors[i].NT;
			if (NT > 0) {
				float T = Sensors[i].TT / NT;
			
				y += 10;
				#ifdef DEBUG_CONTROL
					Serial.print(T);
					Serial.print(" : ");
					Serial.println(SetPoints[channel]);
				#endif
				
				if (T < SetPoints[channel] - Hysteresis) {
					bitSet(Output, channel);
					/*if (Sensors[i].TimeOn == 0) {
						Sensors[i].TimeOn = now;
					}*/
				}
				if (T > SetPoints[channel] + Hysteresis) {
					bitClear(Output, channel);
					/*if (Sensors[i].TimeOn != 0) {
						Sensors[i].TotalTimeOn += (Now-Sensors[i].TimeOn);
						Sensors[i].TimeOn = 0;
					}*/		
				}
				// show Output on display
				String S = String(T);
				S += "   ";
				S += String(SetPoints[channel]);
				char buf[20]; 
				strcpy(buf,S.c_str());
				showMessage(0,y,buf);
			}	
		}
	}
		
	if (Auto) {
		bitSet(Output, 7);
	}
	else
	{
		bitClear(Output, 7);
	}	

	#ifdef DEBUG_CONTROL
		Serial.println(Output,BIN);
		String S = String(Output, BIN);
		char buf[20]; 
		strcpy(buf,S.c_str());
		showMessage(64,0,buf);
	#endif
	Wire.beginTransmission(CONTROL);
	Wire.write(Output);
	Wire.endTransmission(); 
	Wire.beginTransmission(CONTROL);
	Wire.write(Output);
	Wire.endTransmission();   
}

void UpdateDay(int Day){    // tested OK
	#ifdef DEBUG_UPDATE_DAY
		Serial.println("Before Update Day");
		showMem();
	#endif	
	File configFile;
	for (int s = 0; s < (int)Sensors.size(); s++) {	
		int channel = Sensors[s].Channel-1; 
		if (channel > -1) {
			char buf[25];
			sprintf(buf,"/C%dD%d.jsn",channel,Day-1);
			#ifdef DEBUG_UPDATE_DAY
				Serial.println(buf);
				printChannelPairs(s);
				setLocalTime();
				Serial.print(hour(local));
				Serial.print(":");
				Serial.println(minute(local));
			#endif	
			configFile = SPIFFS.open(buf,"r");
			if (configFile) {
				StaticJsonBuffer<2000> jsonBuffer;
				// Parse the root object
				JsonObject &root = jsonBuffer.parseObject(configFile);
				if (root.success()) {
					// copy last SP in SP[0];
					int LastValue = 0;
					for ( byte i = 1; i < 9; i++) {
						if (Sensors[s].SP[i] > 0) {
							LastValue = Sensors[s].SP[i];
						}	
					}
					Sensors[s].Time[0] = 1; // 00:01 
					Sensors[s].SP[0] = LastValue;
					// read setpoints	
					int SPCount =  root["SetPoints"].size();
					for ( byte i = 0; i < SPCount; i++) {
						int MM = 0;
						int HH = 0;	
						sscanf(root["SetPoints"][i]["H"].as<char*>(),"%d:%d",&HH,&MM);
						Sensors[s].Time[i+1] = HH*60+MM;   
						Sensors[s].SP[i+1] = root["SetPoints"][i]["T"].as<float>() * 100;
						Serial.println(Sensors[s].SP[i+1]);
					}
					if (Sensors[s].SP[0] == 0) {
						// copy last SP in SP[0];
						int LastValue = 0;
						for ( byte i = 1; i < 9; i++) {
							if (Sensors[s].SP[i] > 0) {
								LastValue = Sensors[s].SP[i];
								
							}
						}	
					}
					Sensors[s].Time[0] = 1; // 00:01 
					Sensors[s].SP[0] = LastValue;
					printSensors(s);
				}
				configFile.close();
				#ifdef DEBUG_UPDATE_DAY
					printChannelPairs(s);	
				#endif
			}
			else
			{
				#ifdef DEBUG_UPDATE_DAY
					Serial.println("file error : ");
				#endif	
			}
		}			
	}
	#ifdef DEBUG_UPDATE_DAY
		Serial.println("After UpdateDay");
	#endif	
}	

void UpdateSetPoints() {				// Tested 0K
	#ifdef DEBUG_UPDATE_SETPOINTS
		Serial.println(F("Before Update SetPoints"));
		printSetPoints();
		char buf[25];
		sprintf(buf,"%04d-%02d-%02d %02d:%02d:%02d",year(local),month(local),day(local),hour(local),minute(local),second(local));
        Serial.println(buf);
	#endif
	setLocalTime();
	int T = hour(local)*60+minute(local);		
	for (int s = 0; s < (int)Sensors.size(); s++) {
		int channel = Sensors[s].Channel;
		if (channel > 0) { 
			#ifdef DEBUG_UPDATE_SETPOINTS
				//Serial.println(channel);
			#endif	
			int i = 0;	
			while ((i<9) && (T >= Sensors[s].Time[i])) {
				i++; 	
			}
			//if (i != Sensors[s].NdxSP) {
				// do not update if we are in the same period
				// the set point may be changed by user
				SetPoints[channel-1] = Sensors[s].SP[i]/100;
				Sensors[s].NdxSP = i;
				#ifdef DEBUG_UPDATE_SETPOINTS
					printSensor(s);
				#endif
			//}
		}
	}

	#ifdef DEBUG_UPDATE_SETPOINTS
		Serial.println("After Update SetPoints");
		printSetPoints();
	#endif	
}	
