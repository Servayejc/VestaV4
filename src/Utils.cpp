//2020-01-13

#include <Arduino.h>
#include "Utils.h"

#include "SD.h"
#include <FS.h>
#include "SdFat.h"
#include "SSD1306Wire.h"
#include <ESP8266SMTP.h>
#include "Config.h"
#include "Common.h"
#include "Debug.h"

#include <ArduinoJson.h>

SSD1306Wire  display(0x3c, D1, D2);

/*
void dateTime(uint16_t* date, uint16_t* time) {
  *date = FAT_DATE(year(local),month(local),day(local));
  *time = FAT_TIME(hour(local), minute(local), second(local));
}*/

void initSDcard() {
  
  if (SD.begin(D8)) {
	//SdFile::dateTimeCallback(dateTime);
  } else {
	Serial.println(F("initialization failed!"));
	while (1);
  }
}

void initSPIFFS() {
	SPIFFS.begin();
}

void initDisplay() {
	display.init();
	Wire.setClock(100000);
	display.flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
}

//Serial.println("Utils");
long ET = millis();

void startElapsedTime() {
	ET = millis();
}

void elapsedTime() { 
	Serial.print(F("elapsedTime = "));
	Serial.println(millis() - ET);
	ET = millis();
}

void checkMem() {
#ifdef DEBUG_MEM 
	Serial.print(F("--- Heap size : "));
	Serial.println(ESP.getFreeHeap());
#endif	
}

void showMessage(int X, int Y, char* text) {
	//display.clear();
	display.setColor(BLACK);
	display.fillRect(X, Y, display.width(), Y+10);	
	display.setColor(WHITE);
	display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(X, Y, 100, text);
	display.display();
}

void showMem() {
	char heap[10];
	sprintf(heap,"%d",ESP.getFreeHeap());
	showMessage(display.width()/2+20,0,heap);
}

void saveFile(String fileName, String Data, bool OnSD ){
	// Save file on SD card
	Serial.println(Data);
	File dataFile;
	if (OnSD) { 
		SD.remove(fileName);
		dataFile = SD.open(fileName,FILE_WRITE);
	} else {	
		SPIFFS.remove(fileName);
		dataFile = SPIFFS.open(fileName.c_str(),"w");
	}
	
	if (dataFile) { 
		dataFile.println(Data);
		Serial.println(Data);
		dataFile.close();
		#ifdef DEBUG_SERVER
			Serial.print(F("File closed "));
			Serial.println(fileName);
		#endif	
	}	
}

void sendMail(const char* text){
	#ifdef SEND_MAIL
		SMTP.setEmail(_Mail_User);
			.setPassword(_Mail_Pass);
			.Subject("ESP8266 Alert")
			.setFrom(_Mail_From)
			.setForGmail();						
		if(SMTP.Send(_Mail_Dest, text)) {
			Serial.println(F("Message sent"));
		} else {
			Serial.print(F("Error sending message: "));
			Serial.println(SMTP.getError());
		} 
	#endif	
} 

void printSetPoints() {
	Serial.print(F("SetPoints: "));
	for ( int i = 0; i < 6; i++) {   // 6 channels
		Serial.print(SetPoints[i]);
		Serial.print(" ");
	}
	Serial.println();	
}

void printSensor(int Ndx){
	Serial.print("Address:");
	for(int i=0; i < 8; i++) {
	  Serial.print(Sensors[Ndx].Address[i],HEX);
	  Serial.print (" ");	
	}
	Serial.print(" Port:");
	Serial.print(Sensors[Ndx].Port);
	Serial.print (" Chan: ");
	Serial.print(Sensors[Ndx].Channel);
	Serial.print(" SP:");
	printChannelPairs(Ndx);
	Serial.println();
}

void printChannelPairs(int i){				// tested
	for (int p = 0; p < 9 ; p++) {
		char buf[100];
		int T = Sensors[i].Time[p];
		int H = T / 60;
		int M = T % 60;
		float SP = Sensors[i].SP[p];
		sprintf(buf,"%02d:%02d=%5.2f ",H,M,SP/100);
		Serial.print(buf);	
	}
	Serial.println();
}

int getStationByPort(int Port) {
	for (int s = 0; s < (int) Stations.size(); s++) { 
		if (Stations[s].Port == Port) {
			return s;
		}
	}
	return -1;	
}

String GetRemoteStationURL(int Port){
	for (int s = 0; s < (int) Stations.size(); s++) { 
		if (Stations[s].Port == Port) {
			return Stations[s].URL;
		}
	}
	return "";
}

bool HasValues(){
	bool result = false;
	for (byte i = 0; i < Sensors.size(); i++) {
		if (Sensors[i].NT > 0) {
		   result = true;
		   break;
		}
	}
	return result;	
}

void printSensors(int i) {
	Serial.println("--------------------------------");
	Serial.println(Sensors[i].Desc);
	for (byte j = 0; j < 8; j++) {
		Serial.print(Sensors[i].Address[j],HEX);
	}	
	Serial.println();
	Serial.println(Sensors[i].Port);
	Serial.println(Sensors[i].Channel);
	Serial.println(Sensors[i].Ready);
	Serial.println(Sensors[i].TT);
	Serial.println(Sensors[i].NT);
	Serial.println(Sensors[i].NdxSP);
	for (byte j = 0; j < 9; j++) {
	  	Serial.print(Sensors[i].Time[j]);
		Serial.print(" : ");  
		Serial.println(Sensors[i].SP[j]);
	}
}

bool logDataOnSD(char* fileName, char* Data) {
	Serial.println(fileName);
	Serial.println(Data);
	bool newfile = !SD.exists(fileName);
	File dataFile = SD.open(fileName,FILE_WRITE);
	if (dataFile) { 
		if (newfile) {
			dataFile.print("{\"Data\":[");
		}else
		{
			int pos = dataFile.position();
			dataFile.seek(pos-2);
			dataFile.print(",");
		}
		dataFile.print(Data);
		dataFile.print("]}");
		dataFile.close();
		return true;
	}
	return false;
}

bool archiveData() {
	setLocalTime();	
	#ifdef DEBUG_MQTT_SEND_MESSAGE
		checkMem();
		char localTime[25];
		sprintf(localTime,"%04d-%02d-%02d  %02d:%02d:%02d",year(local),month(local),day(local),hour(local),minute(local),second(local));
		Serial.print(F("Sending at "));
		Serial.println(localTime);
	#endif	
	// prepare TimeStamp
	char utc [25];
	sprintf(utc,"%04d-%02d-%02dT%02d:%02d:%02d",year(),month(),day(),hour(),minute(),second());  // UTC time
	// Prepare json
	StaticJsonBuffer<2000> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	// Add Data
	root["ID"]="ESP8266";	
	root["UID"] = utc;
	String S = "";
	for (byte i = 0; i < Sensors.size(); i++) {
		if (Sensors[i].NT > 1)  {
			//if (Sensors[i].TT > -100)  {
				#ifdef  DEBUG_MQTT_SEND_MESSAGE
					Serial.print(S);
					Serial.print (" : ");
					Serial.println(Sensors[i].TT);
				#endif
				
				if (Sensors[i].NT > 1) {  // don't send if there is no update in the quarter
					S = "T_" + String(i);
					Serial.println(S);
					root[S] = Sensors[i].TT / (Sensors[i].NT);
					Sensors[i].TT = Sensors[i].TT / (Sensors[i].NT);
					Sensors[i].NT = 1;
				}
			//}
		}
	}
	#ifdef DEBUG_MQTT_SEND_MESSAGE
		Serial.print(F("Payload : "));
		root.prettyPrintTo(Serial);
		Serial.println();
	#endif	
	char payload[256];
	root.printTo(payload, sizeof(payload));

	Serial.println(payload);
	#ifdef LOGDATA_ON_SD
		char fileName[25];
		sprintf(fileName,"%02d%02d.JSN",month(local),day(local));
		logDataOnSD(fileName,payload);
		Serial.println(payload);
	#endif
	checkMem();
  return true;		
}





