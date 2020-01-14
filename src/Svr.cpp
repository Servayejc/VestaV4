//2020-01-12
#include "SVR.h"
#include <Arduino.h>
#include <FS.h>
#include "SD.h"
#include "Utils.h"
#include <ArduinoJson.h>
#include "Common.h"
#include "RTC.h"
#include "Control.h"
#include "Debug.h"


ESP8266WebServer server(80);
bool HeaderOK = false;
bool clientIsLocal = false;

void processClient(){
    server.handleClient();
}

void sendHeader(int ContentLength = 0) {							// tested	
	server.sendHeader("Access-Control-Allow-Origin","*");
	server.sendHeader("Access-Control-Allow-Methods","POST,GET,OPTIONS");
	server.sendHeader("Access-Control-Allow-Headers","Origin,X-Requested-With,Content-Type, Accept");
	server.sendHeader("Origin","http://servaye.com");
	if (ContentLength != 0) {
		server.setContentLength(ContentLength);
	}
}

int sendResponse(String payload) {							// tested
	sendHeader(payload.length());
	int HTTPCode = 200;
	server.send(HTTPCode, "text/json", payload);
	#ifdef DEBUG_SERVER 
		Serial.print(payload);	
		Serial.println(F("Sended "));
	#endif	
	return HTTPCode; 	
}

void ReadFile(String filename){
	File dataFile = SD.open(filename);
	if (dataFile) { 
		StaticJsonBuffer<2000> jsonBuffer;
		JsonObject &root = jsonBuffer.parseObject(dataFile);
		root.prettyPrintTo(Serial);
		dataFile.close();
	}	
}

void sendFileContent(String filename, bool FromSD) {
	Serial.println(filename);
	File dataFile;
	if (FromSD) { 
		dataFile = SD.open(filename,FILE_READ);
	} else {	
		dataFile = SPIFFS.open(filename.c_str(),"r");
	}
	
	if (dataFile) { 
		sendHeader(dataFile.size()+1);
		server.streamFile(dataFile, "text/html");
		dataFile.close();
		#ifdef DEBUG_SERVER
			Serial.print(F("file closed : "));
			Serial.println(filename);
		#endif
	}
	else
	{
		server.send(200, "ContentType", "File not Found");

		#ifdef DEBUG_SERVER
			Serial.print(F("file error in sendFileContent : "));
			Serial.println(filename);
		#endif	
	}		
}

/* ======================= Remotes ============================*/

int handleGetRemoteConfig() {  // called by remote station
	int portNdx = server.arg("remote").toInt();
	portNdx = 1;
	#ifdef DEBUG_SERVER
		Serial.print("Remote config from slave # ");
		Serial.println(portNdx);
  #endif
	
	StaticJsonBuffer<2000> jb;
	JsonObject& root = jb.createObject();
	root["time"] = now();
	JsonArray& sensors = root.createNestedArray("Sensors");
	for (int i = 0; i < (int)Sensors.size(); i++){
		// send only remote sensors for this slave
		if (Sensors[i].Port == portNdx) {
			JsonObject& sensor = sensors.createNestedObject();
			sensor["Ndx"] = i;
			JsonArray& address = sensor.createNestedArray("address");
			for (int j = 0; j < 8; j++){
				//address.add(_Sensors[i][j]);
				address.add(Sensors[i].Address[j]);
			}		
		}
	}
	String payload; 
	root.printTo(payload);
	server.send(200,"text/plain",payload);
	#ifdef DEBUG_SERVER
		root.prettyPrintTo(Serial);
		Serial.println();	
	#endif
	return sendResponse(payload);
}

int handleSaveRemoteData() {  // called by remote station
	#ifdef DEBUG_SERVER
		Serial.println("Save remote data");
	#endif
	StaticJsonBuffer<2000> jsonBuffer;
	JsonObject &root = jsonBuffer.parseObject(server.arg("plain"));
	if (root.success()){
		#ifdef DEBUG_SERVER
			root.prettyPrintTo(Serial);
			Serial.println();
		#endif
		// Extract values
		//	{"Sensors":[{"Bat":4.539795,"SOC":159.832,"Ala":"No","Ndx":6,"TT":2306.25,"NT":1}]}

		JsonArray& sensors = root["Sensors"];
		for (int i = 0; i < (int)sensors.size(); i++) {
			int ndx = root["Sensors"][i]["Ndx"];
			Serial.println(ndx);
			Serial.print("----");
			
			float B = root["Sensors"][i]["Bat"];
			Serial.println(B);
			float S = root["Sensors"][i]["SOC"];
			Sensors[ndx].Bat = B;
			Serial.println(B);
       		Sensors[ndx].SOC = S;
	    	
			Sensors[ndx].TT = Sensors[ndx].TT + (int)root["Sensors"][i]["TT"];
	   	 	Sensors[ndx].NT += 1;
			#ifdef DEBUG_BATTERY
				Serial.println("saving battery data");
				//if (root["Sensors"][i].containsKey("SOC")) {
					
					char utc [25];
					sprintf(utc,"%04d-%02d-%02dT%02d:%02d:%02d",year(),month(),day(),hour(),minute(),second());
					// {"UID":"2019-03-25T00:00:00","V":4.5,"SOC":89}
					Serial.println(utc);
					float V = root["Sensors"][i]["Bat"];
					Serial.println(V);
					float SOC = root["Sensors"][i]["SOC"];
					Serial.println(SOC);
					
					char payload[400];
					sprintf(payload,"{\"UID\":\"%s\",\"ID\":%d,\"V\":%f,\"SOC\":%f}",utc,ndx,V,SOC);
					Serial.println(payload);
					
					char fileName[25];
					setLocalTime();
					sprintf(fileName,"SOC_%02d%02d.jsn",month(local),day(local));
					Serial.println(fileName);
					logDataOnSD(fileName,payload);
				//}
				 
			#endif
			#ifdef DEBUG_GET_REMOTE_VALUES
				Serial.print(ndx);
				Serial.print(" : ");
				Serial.println(_TT[ndx]);
			#endif			
	
		}
		server.send(200,"text/plain","Ok");
	}
	return 0;	
}

/* ======================= Root ============================*/

int handleroot(){ // called by local client
	//int payloadSize = getFileSize("index.htm");
	#ifdef DEBUG_SERVER
		Serial.println("handleroot");
	#endif	
	//sendHeader(payloadSize);

	sendFileContent("/index.htm", false);
	server.sendContent("/0");	
	return 200; 
}

/* ======================= SetPoints ============================*/

int handleSetPoints() {  // called by local client  
	//int payloadSize = getFileSize("setpoint.htm");
	#ifdef DEBUG_SERVER
		Serial.print(F("SetPoint"));
	#endif	
	//sendHeader(payloadSize);

	sendFileContent("/setpoint.htm", false);	

	server.sendContent("/0");	
	return 200; 
}

int handleGetSetPoints() {  // called by setpoints.htm     								// tested
	// compute filename
	#ifdef DEBUG_SERVER
		Serial.println("GetSetPoint");
	#endif	
  String fileName = "/";
	if (server.args() == 2){  
		fileName += "C";
		fileName += server.arg(0);
		fileName += "D";
		fileName += server.arg(1);	
		fileName += ".jsn";
	}
	else 
	{ 
		fileName += "/C0D0.jsn";
	}	
	#ifdef DEBUG_SERVER
		Serial.println(fileName);
	#endif	
	//int payloadSize = getFileSize(fileName);
	//sendHeader(payloadSize);
	sendFileContent(fileName, false);
	server.sendContent("/0");	
	return 200; 
}

int handleSaveSetPoints() {  // called by setpoint.htm  								// tested
	#ifdef DEBUG_SERVER
		Serial.println(F("Save setpoint"));
	#endif	
	int HTTPCode = 0;
	String Data = server.arg(0);
	StaticJsonBuffer<2000> jsonBuffer;
	JsonObject &root = jsonBuffer.parseObject(Data);
	if (root.success()){
		// compute filename(s)
		//int Days = root["Channel"];
		int Save = root["Save"];
		for ( byte d = 0; d < 7; d++) {
			if ((Save >> d) & 1) {
				String fileName = "/C";
				fileName += root["Channel"].as<char*>();
				fileName += "D";
				fileName += d;
				fileName += ".jsn";
				#ifdef DEBUG_UPDATE_SETPOINTS
				    Serial.println(fileName);
					Serial.println(Data);
				#endif	
			
				saveFile(fileName, Data, false);
				fileName = "";
				
			}	
			
		}		

		HTTPCode = sendResponse("Data saved");
		//ConfigChanged = true;
	    //setLocalTime();
		//UpdateDay (weekday(local));
		//UpdateSetPoints();
		Serial.println("Setpoints saved");
		return HTTPCode;
	}
	else 
	{	
		return HTTPCode;
	}
}	

/* ======================= Status ============================*/

int handleGetStatus() {       								// ready to test
	sendFileContent("/status.htm", false);
	server.sendContent("/0");	
	return 200; 
}

int handleGetStatusData(){	// called by status.htm			// test
	#ifdef DEBUG_SERVER
		Serial.println("Get status data");
	#endif
	String payload = "";
  	if (HasValues()) {
		StaticJsonBuffer<2000> jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();
		// Add Data
		Auto ? root["Mode"]="Auto" : root["Mode"]="Thermostats";
		char buf[25];
		setLocalTime();
		sprintf(buf,"%04d-%02d-%02d   %02d:%02d:%02d",year(local),month(local),day(local),hour(local),minute(local),second(local));
		root["Time"] = buf; 
		#ifdef DEBUG_SERVER
			Serial.print("Sensors count :");
			Serial.println(Sensors.size());
		#endif	
		JsonArray& S = root.createNestedArray("Sensors");
		for (byte i = 0; i < Sensors.size(); i++) {
			#ifdef DEBUG_SERVER
				Serial.print(i);
				Serial.print(" NT = ");
				Serial.println(Sensors[i].NT);
			#endif	
			if (Sensors[i].NT > 1){
				JsonObject& Sensor = S.createNestedObject();
				Sensor["_T"] = Sensors[i].TT/Sensors[i].NT; 
				Sensor["_DSC"] = Sensors[i].Desc;
				Sensor["Ndx"] = Sensors[i].Ndx;
				Sensor["_V"] = Sensors[i].Bat;
				Sensor["_S"] = Sensors[i].SOC;
				int channel = Sensors[i].Channel-1;
				if (Sensors[i].Port == 0) {
					// local
					Output & (1 << channel) ? Sensor["_ST"] =  1 : Sensor["_ST"] = 0;
				} else {
					// remote
					int S = (int)Sensors[i].SOC;
					if (S > 100) {
						S = 100;
					}	
					Sensor["_ST"] = 7 - (S+12)/25; 
				}
 
				if (channel > -1) {
					Sensor["_C"] = channel;
					Sensor["_SP"] = SetPoints[channel];    
				}
			}
		}	
		root.printTo(payload);
		#ifdef DEBUG_SERVER
			Serial.println(payload);
		#endif
	}
	else
	{		
		payload = ("No data available");	
	}	
	return sendResponse(payload);
}

int handleUpdateSetPoints(){
	Serial.println("Update setpoints ---------------------");
	int HTTPCode = 0;
	String Data = server.arg(0);
	Serial.println(Data);
	String Password = server.arg(1);
	if (true) /*(Password = "XYZ")*/ {
		Serial.println(Data);
		StaticJsonBuffer<2000> jsonBuffer;
		JsonObject &root = jsonBuffer.parseObject(Data);
		if (root.success()){
			root.prettyPrintTo(Serial);
			// TODO update setpoints here
			#ifdef DEBUG_SERVER
				printSetPoints();
			#endif	
			int count = root["SetPoints"].size();
			Serial.println(count);
			for ( int i = 0; i < count; i++) {	
				if (root["SetPoints"][i]["_C"].success()) {
					int ChannelNdx = root["SetPoints"][i]["_C"].as<int>();
					float NewSP = root["SetPoints"][i]["_SP"];
					float OldSP = root["SetPoints"][i]["_MO"];
					// update setpoint for regulation
					SetPoints[ChannelNdx] = NewSP;
					// update setpoint in sensor
					int S = root["SetPoints"][i]["Ndx"];
					Sensors[S].SP[Sensors[S].NdxSP] = NewSP * 100;
					#ifdef DEBUG_SERVER
						Serial.print("NewSP :");
						Serial.print(ChannelNdx);
						Serial.print(" =");
						Serial.println(NewSP);
					#endif		
				}
			}
			HTTPCode = sendResponse("Les nouvelles consignes sont appliqu�es.");
			#ifdef DEBUG_SERVER
				printSetPoints();
			#endif		
		}
		else 
		{
		HTTPCode = sendResponse("Mot de passe invalide.");
		}	
		return HTTPCode;
	} 
	else 
	return -1;
}

/* ======================= Log ============================*/

int handleGetLogData() {
	#ifdef DEBUG_SERVER
		Serial.println("LogData");
	#endif	
	String fileName = "";
	if (server.args() == 1){  
		fileName += server.arg(0);	
		fileName += ".JSN";
	}
	else
	{
		return 0;
	}	
	sendFileContent(fileName, true);
	server.sendContent("/0");	
	return 200;	
}

int handleGetLog() {
	#ifdef DEBUG_SERVER
		Serial.println("GetLogs");
	#endif	
	sendFileContent("/temper.htm", false);
	server.sendContent("/0");	
	return 200; 
}

/* ======================= Battery ============================*/

int handleGetBatData() {
	#ifdef DEBUG_SERVER
		Serial.println("BatData");
	#endif	
	String fileName = "";
	if (server.args() == 1){  
		fileName += server.arg(0);	
		fileName += ".JSN";
	}
	else
	{
		return 0;
	}	
	sendFileContent(fileName, true);
	server.sendContent("/0");	
	return 200;	
}

int handleGetBat() {
	#ifdef DEBUG_SERVER
		Serial.println("GetBat");
	#endif	
	sendFileContent("/battery.htm", false);
	server.sendContent("/0");	
	return 200; 
}

int handleSensors() {       								// ready to test
	sendFileContent("/sensors.htm", false);
	server.sendContent("/0");	
	return 200; 
}

/* ======================= Sensors ============================*/

int handleGetSensors() {  // called by setpoints.htm     								// tested
	// compute filename
	#ifdef DEBUG_SERVER
		Serial.println("GetSensors");
	#endif	
    String fileName = "/Sensors.jsn";
	sendFileContent(fileName, false);
	server.sendContent("/0");                                                                                                                                                                           	
	return 200; 
}

int handleUpdateSensors() { 
	#ifdef DEBUG_SERVER
		Serial.println(F("Save sensors"));
	#endif	
	int HTTPCode = 0;
	String Data = server.arg(0);
	Serial.println(Data);	
	StaticJsonBuffer<2000> jsonBuffer;
	JsonObject &root = jsonBuffer.parseObject(Data);
	if (root.success()){
		String fileName = "/Sensors.jsn";
		saveFile(fileName, Data, false);
		HTTPCode = sendResponse("Data saved");
		ConfigChanged = true;
		setLocalTime();
		UpdateDay (weekday(local));
		UpdateSetPoints();
		Serial.println("Sensors saved, restating... ");
		return HTTPCode;
	}
	else 
	{	
		return HTTPCode;
	}
	//readSensorsConfig();
}	

/* ======================= Files ============================*/
int handleFiles() {       								// ready to test
	sendFileContent("/files.htm", false);
	server.sendContent("/0");	
	return 200; 
}

int handleGetFile() {       								// ready to test
		String fileName = "/";
	if (server.args() == 1){  
		fileName += server.arg(0);
		Serial.println(fileName);	
	}
	else
	{
		return 0;
	}	
	sendFileContent(fileName, false);
	server.sendContent("/0");	
	return 200;	
}

/* ======================= Access Level ============================*/

/*int handleGetAccessLevel() {
	if (server.hasArg("plain")== false){ //Check if body received
		server.send(200, "text/plain", "Body not received");
    return 0;
	}
	String Data = server.arg(0);	
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject &root = jsonBuffer.parseObject(Data);
	String id = root("id");
	String pass = root("Pass");
    server.send(200, "text/plain", "254");
		
}*/

/*	
	StaticJsonBuffer<2000> jsonBuffer;
	JsonObject &root = jsonBuffer.parseObject(Data);
	if (root.success()){
	
	}
}*/

void startServer(){
	//Associate the handler functions 

	// root
	server.on("/", handleroot);	
	
	// remotes
	server.on("/getremoteconfig", handleGetRemoteConfig);
	server.on("/saveremotedata", handleSaveRemoteData);	
	
	// setpoints
	server.on("/setpoints", HTTP_GET, handleSetPoints);	
	server.on("/getsetpoints", handleGetSetPoints);	
	server.on("/savesetpoints", handleSaveSetPoints);

	// status
	server.on("/status", handleGetStatus);
	server.on("/savestatusdata", handleUpdateSetPoints);
	server.on("/getstatusdata", handleGetStatusData);  
    
	// logs
	server.on("/getlog", handleGetLog);
    server.on("/getlogdata", handleGetLogData);  
	
	// battery
	server.on("/getbat", handleGetBat);
    server.on("/getbatdata", handleGetBatData);  

	// sensors
	server.on("/sensors", handleSensors);
	server.on("/savesensors", handleUpdateSensors);
	server.on("/getsensors", handleGetSensors);  
	
	// files
	server.on("/files", handleFiles);
	server.on("/getfile", handleGetFile);
	
	// accesslevel
	//server.on("/accesslevel", handleGetAccessLevel);
	
	server.onNotFound([](){server.send(404, "text/plain", "404 - Not found");});
	server.begin();


	

}
