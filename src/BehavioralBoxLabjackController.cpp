/**
 * Name: BehavioralBoxLabjackController.cpp
 * Desc: Main file
**/

//#ifdef _WIN32
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#else
//#include <arpa/inet.h>  // For inet_ntoa()
//#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <LabJackM.h>
#include <time.h>

//#include "../../C_C++_LJM_2019-05-20/LJM_Utilities.h"
#include "BehavioralBoxLabjack.h"


//// Scheduler
#define PX_SCHED_IMPLEMENTATION 1
#include "External/px_sched.h"

bool isArtificialDaylightHours();
double SyncDeviceTimes(BehavioralBoxLabjack* labjack);
void updateVisibleLightRelayIfNeeded(BehavioralBoxLabjack* labjack);


int main()
{
	// Open first found LabJack
	//GetDeviceInfo("LJM_dtANY", "LJM_ctANY", "LJM_idANY");
	BehavioralBoxLabjack firstLabjack = BehavioralBoxLabjack(0, "LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	SyncDeviceTimes(&firstLabjack);

	updateVisibleLightRelayIfNeeded(&firstLabjack);

	Scheduler schd;
	schd.init();

	//WaitForUserIfWindows();

	return LJME_NOERROR;
}

//void GetDeviceInfo(const char * devType, const char * connType, const char * iden)
//{
//	int err;
//	int handle;
//	int portOrPipe, ipAddress, serialNumber, packetMaxBytes;
//	int deviceType = LJM_dtANY;
//	int connectionType = LJM_ctANY;
//	char string[LJM_STRING_ALLOCATION_SIZE];
//
//	printf("LJM_OpenS(\"%s\", \"%s\", \"%s\")\n", devType, connType, iden);
//
//	err = LJM_OpenS(devType, connType, iden, &handle);
//	ErrorCheck(err, "LJM_OpenS");
//
//	// Get device name
//	err = LJM_eReadNameString(handle, "DEVICE_NAME_DEFAULT", string);
//	if (err == LJME_NOERROR)
//		printf("DEVICE_NAME_DEFAULT: %s\n", string);
//	else
//		printf("This device does not have a name\n");
//
//	// Get device info
//	err = LJM_GetHandleInfo(handle, &deviceType, &connectionType, &serialNumber, &ipAddress,
//		&portOrPipe, &packetMaxBytes);
//	ErrorCheck(err, "LJM_GetHandleInfo");
//
//	PrintDeviceInfo(deviceType, connectionType, serialNumber, ipAddress, portOrPipe, packetMaxBytes);
//
//	if (deviceType != LJM_dtDIGIT) {
//		printf("\nETHERNET:\n");
//		GetAndPrintIPAddress(handle, "ETHERNET_IP");
//		GetAndPrintIPAddress(handle, "ETHERNET_SUBNET");
//		GetAndPrintIPAddress(handle, "ETHERNET_GATEWAY");
//		GetAndPrintIPAddress(handle, "ETHERNET_DNS");
//		GetAndPrintIPAddress(handle, "ETHERNET_ALTDNS");
//		GetAndPrint(handle, "ETHERNET_DHCP_ENABLE");
//		GetAndPrint(handle, "POWER_ETHERNET");
//
//		printf("\n");
//		GetAndPrintIPAddress(handle, "ETHERNET_IP_DEFAULT");
//		GetAndPrintIPAddress(handle, "ETHERNET_SUBNET_DEFAULT");
//		GetAndPrintIPAddress(handle, "ETHERNET_GATEWAY_DEFAULT");
//		GetAndPrintIPAddress(handle, "ETHERNET_DNS_DEFAULT");
//		GetAndPrintIPAddress(handle, "ETHERNET_ALTDNS_DEFAULT");
//		GetAndPrint(handle, "ETHERNET_DHCP_ENABLE_DEFAULT");
//		GetAndPrint(handle, "POWER_ETHERNET_DEFAULT");
//		GetAndPrintMACAddressFromValueAddress(handle, "ETHERNET_MAC", 60020);
//
//		if (DoesDeviceHaveWiFi(handle)) {
//			printf("\nWIFI:\n");
//			GetAndPrintIPAddress(handle, "WIFI_IP");
//			GetAndPrintIPAddress(handle, "WIFI_SUBNET");
//			GetAndPrintIPAddress(handle, "WIFI_GATEWAY");
//			GetAndPrint(handle, "WIFI_STATUS");
//			GetAndPrint(handle, "WIFI_DHCP_ENABLE");
//			GetAndPrint(handle, "POWER_WIFI_DEFAULT");
//			GetAndPrint(handle, "WIFI_VERSION");
//			err = LJM_eReadNameString(handle, "WIFI_SSID", string);
//			if (err) {
//				printf("Could not read WIFI_SSID. Error was %d\n", err);
//			}
//			else {
//				printf("WIFI_SSID: %s\n", string);
//			}
//			GetAndPrintMACAddressFromValueAddress(handle, "WIFI_MAC", 60024);
//		}
//	}
//
//	printf("\n");
//	GetAndPrint(handle, "HARDWARE_VERSION");
//	GetAndPrint(handle, "FIRMWARE_VERSION");
//
//	// Time Sync
//	SyncDeviceTimes(handle);
//	
//	CloseOrDie(handle);
//}
//
//// Syncs the Labjack's internal RTC time with the computer's. Returns the number of seconds that were adjusted to set the Labjack's clock.
double SyncDeviceTimes(BehavioralBoxLabjack* labjack) {
	int LJMError;
	time_t originalLabjackTime = labjack->getTime();
	LJMError = labjack->getError();
	printf("LABJACK TIME: %s\n", ctime(&originalLabjackTime));
	
	// Get the computer time:
	time_t computerTime;
	time(&computerTime);  /* get current time; same as: timer = time(NULL)  */
	printf("COMPUTER TIME: %s\n", ctime(&computerTime));

	double updateChangeSeconds = difftime(computerTime, originalLabjackTime);

	if (updateChangeSeconds == 0) {
		printf("Computer time is already synced with Labjack time!\n");
	}
	else {
		printf("Computer time is %.f seconds from Labjack time...\n", updateChangeSeconds);
		// Write the computer time to the Labjack
		labjack->setTime(computerTime);
		LJMError = labjack->getError();

		// Re-read the time to confirm the update
		time_t updatedLabjackTime = labjack->getTime();
		LJMError = labjack->getError();
		printf("Updated Labjack TIME: %s\n", ctime(&updatedLabjackTime));
	}
	return updateChangeSeconds;
}

bool isArtificialDaylightHours() {
	time_t currTime = time(NULL);
	struct tm *currLocalTime = localtime(&currTime);

	int hour = currLocalTime->tm_hour;
	if ((hour < 6) || (hour > 18)) {
		// It's night-time
		return false;
	}
	else {
		// It's day-time
		return true;
	}	
}


void updateVisibleLightRelayIfNeeded(BehavioralBoxLabjack* labjack) {
	bool isDay = isArtificialDaylightHours();
	labjack->setVisibleLightRelayState(isDay);
}