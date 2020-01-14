//2020-01-12
#include "VESTA.h"
#include <WiFiManager.h>
#include <EasyDDNS.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

#define CONTROL  0x38
#define DISPLAY_ADD  0x3A 
				
SensorsList Sensors = {};
StationsList Stations = {};
ChannelList Channels = {} ;

int LastMin = 0;
bool DayChanged = false;
bool TempReaded = false;
bool ConfigChanged = false;
long connection = 0;
int arrivedcount = 0;
bool Sended = false;

float SetPoints[6]={10,11,12,13,14,15};		// default values, set by wifi 
float Hysteresis = 0.125;	
byte Output = 0;
bool Auto = true;

void configModeCallback (WiFiManager *myWiFiManager) {		// tested
	#ifdef DEBUG_ESP_REMOTE
	Serial.println(F("Entered configuration mode"));
	Serial.println(WiFi.softAPIP());
	Serial.println(myWiFiManager->getConfigPortalSSID());
	#endif
}  

void setup() {
	Serial.begin(74880); 
	delay(1000);
	pinMode(LED_BUILTIN, OUTPUT);
	Serial.println("Start");
	
	#ifdef DEBUG_UPDATE_DAY
		pinMode(D4,INPUT_PULLUP);
	#endif	
	
	// build local URL for mDNS	and wifiManager
	//uint8_t mac[6];	
	Serial.println(WiFi.macAddress());
	char HostString[20]; 
	sprintf(HostString,"VESTA");
	
	WiFiManager wifiManager;	
	//reset saved settings for tests
	// wifiManager.resetSettings(); 
	wifiManager.setAPCallback(configModeCallback);
	wifiManager.autoConnect(HostString);
	
	startElapsedTime();
	Serial.print(F("MDNS responder started "));
	Serial.println(HostString);
	Serial.println(WiFi.localIP());

	if (MDNS.begin(HostString, WiFi.localIP())) {
		MDNS.addService("http", "tcp", 80);
	}
	initDisplay();
	showMessage(0,0,HostString);
	showMem();
		
	pinMode(D8, OUTPUT);

	// démare le système de fichiers
   	//SPIFFS.begin();
	initSDcard();
	initSPIFFS();
	

	readSensorsConfig();
	readStationsConfig();
	
	startSystemTime();
	setLocalTime();	
	elapsedTime();
	

	
	/*Check if remote is present
		if present --- force remote reboot
		if a remote is not present scan network periodically until all remotes are present
	*/
	#ifdef DEBUG_TEST_CONTROL
		ControlTest(); 	
	#endif
	
   
	//start DS18B20 conversion
	StartConversion();
	
	//findRemoteStations();	
	//SetAllRemoteConfigs();	
	
	UpdateDay(weekday(local));
	UpdateSetPoints();
	ConfigChanged = false;
	
	startServer();

	ArduinoOTA.onStart([]() {
    	String type;
    	if (ArduinoOTA.getCommand() == U_FLASH) {
    	  	type = "sketch";
    	} else { // U_SPIFFS
      		type = "filesystem";
    	}

    	// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    		Serial.println("Start updating " + type);
 	});
 
 	ArduinoOTA.onEnd([]() {
    	Serial.println("\nEnd");
  	});
  	
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    	Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  	});
  
  	ArduinoOTA.onError([](ota_error_t error) {
    	Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
  	});
  



  	ArduinoOTA.begin();
  	Serial.println("Ready");
  	Serial.print("IP address: ");
  	Serial.println(WiFi.localIP());
 

	EasyDDNS.service("noip");    
	EasyDDNS.client("http://dynupdate.no-ipcom/nic/update","jcservaye","Onyx2013");    	

    sendMail("Restarted");
	
	#ifdef DEBUG_SETUP	
		// affiche le contenu du système de fichier
		Dir dir = SPIFFS.openDir("/");
		while (dir.next()) {
			Serial.println(dir.fileName());
		}
			Serial.print(F("Setup Done : "));
			checkMem();
	#endif
}		

void loop(void) {
	
	ArduinoOTA.handle(); 
	EasyDDNS.update(10000);
    MDNS.update();
    processClient();
	if (ConfigChanged) {
		#ifdef DEBUG_MAIN
			Serial.println("Config Changed");
		#endif	
		setLocalTime();
		UpdateDay (weekday(local));
		UpdateSetPoints();
		ConfigChanged = false;
		Serial.println("End of Config Changed");
	}	
	//keepMQTTconnected();
	if ((timeStatus() != timeNotSet) && ( year(local) != 1970)) {
		int Secs = 10;
		#ifdef DEBUG_TEST_CONTROL
			Secs = 2;
		#endif
		
		#ifdef DEBUG_UPDATE_DAY
			if (digitalRead(D4) == 0) {
				setLocalTime();
				UpdateDay(weekday(local));
				delay(2000);
			}	  
		#endif

		if (hour(local) == 0 ) {
			setLocalTime();
			// load setpoints for the new day 
			if (!DayChanged) {
				#ifdef DEBUG_MAIN
					Serial.println("Day Changed");
				#endif	
				UpdateDay(weekday(local));
				DayChanged = true;
				#ifdef DEBUG_MAIN
					Serial.println("End of Day Changed");
				#endif
			}
			else 
			{
				DayChanged = false;	
			}
		}	
		
		if ((second() % Secs) == 0) {		
			if (!TempReaded) {
				#ifdef DEBUG_MAIN
					Serial.println("Read Temp");
				#endif	
				ReadTemperatures();
				TempReaded = true;
				ControlTemp();
				#ifdef DEBUG_MAIN
					Serial.println("End of Read Temp");
				#endif	
			}
		}	
		else
		{
			TempReaded = false;
		}
	
		
		if (minute() != LastMin){	
			#ifdef DEBUG_MAIN
				Serial.println("Minute");
			#endif	
			//startElapsedTime();
			UpdateSetPoints();
			LastMin = minute(local);
			//elapsedTime();
			#ifdef DEBUG_MAIN
				Serial.println("End of Minute");
			#endif	
		}	
		
		
		// send data every 15 minutes 
		if ((minute(local)%15) == 0) {   
			
			if (!Sended) {
				#ifdef DEBUG_MAIN
					Serial.println("15 Minutes");
				#endif	
				Sended = archiveData();
				#ifdef DEBUG_MAIN
					Serial.println("End of 15 Minutes");
				#endif
			}
		}
		else
		{
			Sended = false;
		}

	}
	else
	{
		Serial.println("Time not set");
	}	 
} 


 


