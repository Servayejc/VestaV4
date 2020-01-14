//2020-01-12
#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>
#include "RTC.h"


//#include <SPIFFS.h>

void startElapsedTime();
void elapsedTime();
void checkMem();


void initSPIFFS();
void initSDcard(); 
void initDisplay();
void showMessage(int X, int Y, char* text);
void showMem();
void printSensor(int Ndx);
void printDateTime(time_t t, const char *tz);
void printSetPoints();
void printChannelPairs(int i);
void printSensors(int i);
void saveFile(String fileName, String Data, bool OnSD);
void sendMail(const char* text);
//void configModeCallback (WiFiManager *myWiFiManager);
bool HasValues();

int getStationByPort(int Port);

bool logDataOnSD(char* fileName, char* Data);
bool archiveData();

String GetRemoteStationURL(int Port);

void printSD();


#endif