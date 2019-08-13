#include "BehavioralBoxLabjack.h"
/**
 * Name: BehavioralBoxLabjack.cpp
 * Desc: An object representing a single labjack used inside a behavioral box
**/

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>  // For inet_ntoa()
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <LabJackM.h>
#include <time.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>

#include "../../C_C++_LJM_2019-05-20/LJM_Utilities.h"



BehavioralBoxLabjack::BehavioralBoxLabjack(int uniqueIdentifier, int devType, int connType, int serialNumber) : BehavioralBoxLabjack(uniqueIdentifier, NumberToDeviceType(devType), NumberToConnectionType(connType), serialNumber) {}

// Constructor: Called when an instance of the object is about to be created
BehavioralBoxLabjack::BehavioralBoxLabjack(int uniqueIdentifier, const char * devType, const char * connType, int serialNumber): deviceType(LJM_dtANY), connectionType(LJM_ctANY), csv(CSVWriter(",")), lastCaptureComputerTime(Clock::now())
{
	this->serialNumber = serialNumber;
	char iden[256];
	sprintf(iden, "%d", this->serialNumber);
	
	// Open the LabjackConnection and load some information
	this->uniqueIdentifier = uniqueIdentifier;
	this->err = LJM_OpenS(devType, connType, iden, &this->handle);
	this->printIdentifierLine();
	ErrorCheck(this->err, "LJM_OpenS");

	char string[LJM_STRING_ALLOCATION_SIZE];

	// Get device name
	this->err = LJM_eReadNameString(this->handle, "DEVICE_NAME_DEFAULT", string);
	if (this->err == LJME_NOERROR)
		printf("\t DEVICE_NAME_DEFAULT: %s\n", string);
	else
		printf("\t This device does not have a name\n");

	// Get device info
	this->err = LJM_GetHandleInfo(this->handle, &deviceType, &connectionType, &this->serialNumber, &ipAddress,
		&portOrPipe, &packetMaxBytes);
	ErrorCheck(this->err, "LJM_GetHandleInfo");

	//this->diagnosticPrint();

	// File Management
	// Build the file name
	this->lastCaptureComputerTime = Clock::now();
	auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(this->lastCaptureComputerTime);
	auto fraction = this->lastCaptureComputerTime - seconds;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
	unsigned long long milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(this->lastCaptureComputerTime.time_since_epoch()).count();

	// Builds the filename in the form "out_file_s{SERIAL_NUMBER}_{MILLISECONDS_SINCE_EPOCH}"
	std::ostringstream os;
	os << "out_file_s" << this->serialNumber << "_" << milliseconds_since_epoch << ".csv";
	this->filename = os.str();
	// Build the full file path
	if (this->outputDirectory.empty()) {
		this->fileFullPath = this->filename;
	}
	else {
		this->fileFullPath = this->outputDirectory + this->filename;
	}
	std::cout << "\t New file path: " << this->fileFullPath << std::endl;
	
	// Write the header to the .csv file:
	this->csv.newRow() << "computerTime";
	for (int i = 0; i < NUM_CHANNELS; i++) {
		this->csv << this->inputPortNames[i];
	}
	this->csv.writeToFile(fileFullPath, false);

	// Setup output ports states:
	this->outputPorts = {};
	// Create output ports for all output ports (TODO: make dynamic)
	for (int i = 0; i < NUM_OUTPUT_CHANNELS; i++) {
		std::string portName = std::string(outputPortNames[i]);
		this->outputPorts.push_back(&OutputState(portName));
	}

	// Setup input state 
	this->monitor = new StateMonitor();

	// Create the object's thread at the very end of its constructor
	// wallTime-based event scheduling:
	this->scheduler = new Bosma::Scheduler(max_n_threads);

	// Ran at the top of every hour
	this->scheduler->cron("0 * * * *", [this]() { this->runTopOfHourUpdate(); });

	// Start a 20Hz (50[ms]) loop to read data.
	this->scheduler->every(std::chrono::milliseconds(50), [this]() { this->runPollingLoop(); });
}

// Destructor (Called when object is about to be destroyed
BehavioralBoxLabjack::~BehavioralBoxLabjack()
{
	// Stop the main run loop
	this->shouldStop = true;
	//Read the values and save them one more time, so we know when the end of data collection occured.
	this->readSensorValues();

	// Destroy the object's thread at the very start of its destructor
	delete this->scheduler;
	//this->scheduler = NULL;

	// Close the open output file:
	//this->outputFile.close();
	this->csv.writeToFile(this->fileFullPath, true);

	// Close the connection to the labjack
	this->err = LJM_Close(this->handle);
	ErrorCheck(this->err, "LJM_Close");

	delete this->monitor;
	//CloseOrDie(this->handle);
}

void BehavioralBoxLabjack::diagnosticPrint()
{
	this->printIdentifierLine();
	PrintDeviceInfo(deviceType, connectionType, serialNumber, ipAddress, portOrPipe, packetMaxBytes);
	printf("\n");
	GetAndPrint(handle, "HARDWARE_VERSION");
	GetAndPrint(handle, "FIRMWARE_VERSION");
}

// Prints the line that uniquely identifies this labjack
void BehavioralBoxLabjack::printIdentifierLine()
{
	cout << ">> Labjack [" << this->serialNumber << "] :" << endl;
}

void BehavioralBoxLabjack::diagnosticPrintLastValues()
{
	this->printIdentifierLine();
	unsigned long long milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(this->lastCaptureComputerTime.time_since_epoch()).count();
	std::cout << "\t " << milliseconds_since_epoch;
	for (int i = 0; i < NUM_CHANNELS; i++) {
		//if (inputPortValuesChanged[i] == true) {
		//	// The input port changed from the previous value

		//}
		std::cout << "\t" << this->lastReadInputPortValues[i];
	}
	std::cout << std::endl;
}

int BehavioralBoxLabjack::getError()
{
	return this->err;
}

time_t BehavioralBoxLabjack::getTime()
{
	double labjackTime = 0.0;
	this->err = LJM_eReadAddress(this->handle, 61500, 1, &labjackTime);
	ErrorCheck(this->err, "getTime - LJM_eReadAddress");
	return time_t(labjackTime);
}

void BehavioralBoxLabjack::setTime(time_t newTime)
{
	// Write the provided time to the Labjack
		//RTC_SET_TIME_S: address 61504
	this->err = LJM_eWriteAddress(handle, 61504, LJM_UINT32, newTime);
	ErrorCheck(this->err, "LJM_eWriteAddress");
}

double BehavioralBoxLabjack::syncDeviceTimes()
{
	int LJMError;
	time_t originalLabjackTime = this->getTime();
	LJMError = this->getError();

	this->printIdentifierLine();
	printf("\t LABJACK TIME: %s", ctime(&originalLabjackTime));
	// Get the computer time:
	time_t computerTime;
	time(&computerTime);  /* get current time; same as: timer = time(NULL)  */
	printf("\t COMPUTER TIME: %s", ctime(&computerTime));

	double updateChangeSeconds = difftime(computerTime, originalLabjackTime);

	if (updateChangeSeconds == 0) {
		printf("\t Computer time is already synced with Labjack time!\n");
	}
	else {
		printf("\t Computer time is %.f seconds from Labjack time...", updateChangeSeconds);
		// Write the computer time to the Labjack
		this->setTime(computerTime);
		LJMError = this->getError();

		// Re-read the time to confirm the update
		time_t updatedLabjackTime = this->getTime();
		LJMError = this->getError();
		printf("\t Updated Labjack TIME: %s\n", ctime(&updatedLabjackTime));
	}
	return updateChangeSeconds;
}

void BehavioralBoxLabjack::setVisibleLightRelayState(bool isOn)
{
	// Set up for setting DIO state
	this->printIdentifierLine();
	double value = 0; // Output state = low (0 = low, 1 = high)
	char * portName = globalLabjackLightRelayPortName;
	if (isOn) {
		// It's day-time
		value = 0;
	}
	else {
		// It's night-time
		value = 1;
	}
	// Set DIO state on the LabJack
	this->err = LJM_eWriteName(this->handle, portName, value);
	ErrorCheck(this->err, "LJM_eWriteName");
	printf("\t Set %s state : %f\n", portName, value);
}

void BehavioralBoxLabjack::writeOutputPinValues()
{
	this->writeOutputPinValues(false);
}
void BehavioralBoxLabjack::writeOutputPinValues(bool shouldForceWrite)
{
	auto writeTime = Clock::now();

	//Loop through and write the values that have changed
	// Iterate through the output ports
	for (int i = 0; i < outputPorts.size(); i++)
	{
		// Get the appropriate value for the current port (TODO: calculate it).
		int outputValue = 1;

		// Check to see if the value changed, and if it did, write it.
		bool didChange = outputPorts[i]->set(writeTime, outputValue);

		if (didChange || shouldForceWrite) {
			// Get the c_string name to pass to the labjack write function
			const char* portName = outputPorts[i]->pinName.c_str();

			// Set DIO state on the LabJack
			this->err = LJM_eWriteName(this->handle, portName, outputValue);
			ErrorCheck(this->err, "LJM_eWriteName");
			printf("\t Set %s state : %f\n", portName, outputValue);
		}
	}

}

void BehavioralBoxLabjack::readSensorValues()
{
	//time(&this->lastCaptureComputerTime);  /* get current time; same as: timer = time(NULL)  */
	this->lastCaptureComputerTime = Clock::now();

	//Read the sensor values from the labjack DIO Inputs
	this->err = LJM_eReadNames(this->handle, NUM_CHANNELS, (const char **)this->inputPortNames, this->lastReadInputPortValues, &this->errorAddress);
	ErrorCheckWithAddress(this->err, this->errorAddress, "readSensorValues - LJM_eReadNames");
	// Only persist the values if the state has changed.
	if (this->monitor->refreshState(this->lastCaptureComputerTime, this->lastReadInputPortValues)) {
		this->persistReadValues(true);
	}
}

// Reads the most recently read values and persists them to the available output modalities (file, TCP, etc) if they've changed or it's needed.
void BehavioralBoxLabjack::persistReadValues(bool enableConsoleLogging)
{
	unsigned long long milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(this->lastCaptureComputerTime.time_since_epoch()).count();
	CSVWriter newCSVLine(",");

	if (enableConsoleLogging) {
		this->printIdentifierLine();
		cout << "\t " << milliseconds_since_epoch << ": ";
	}
	newCSVLine.newRow() << milliseconds_since_epoch;
	for (int i = 0; i < NUM_CHANNELS; i++) {
		inputPortValuesChanged[i] = (this->lastReadInputPortValues[i] != this->previousReadInputPortValues[i]);
		if (inputPortValuesChanged[i] == true) {
			// The input port changed from the previous value

		}
		newCSVLine << this->lastReadInputPortValues[i];
		if (enableConsoleLogging) {
			cout << this->lastReadInputPortValues[i] << ", ";
		}
		// After capturing the change, replace the old value
		this->previousReadInputPortValues[i] = this->lastReadInputPortValues[i];
		
	}
	if (enableConsoleLogging) {
		cout << std::endl;
	}
	newCSVLine.writeToFile(fileFullPath, true); //TODO: relies on CSV object's internal buffering and writes out to the file each time.
}


// The main run loop
void BehavioralBoxLabjack::runPollingLoop()
{
	if (this->shouldStop) {
		// Stop running the main loop!
		printf("Stopping Labjack %d", this->uniqueIdentifier);
	}
	else {
		this->readSensorValues();
		this->writeOutputPinValues();
	}
}

// Executed every hour, on the hour
void BehavioralBoxLabjack::runTopOfHourUpdate()
{
	time_t computerTime;
	time(&computerTime);  /* get current time; same as: timer = time(NULL)  */
	printf("runTopOfHourUpdate: running at %s for labjack %i\n", ctime(&computerTime), this->serialNumber);
	this->updateVisibleLightRelayIfNeeded();
}

bool BehavioralBoxLabjack::isArtificialDaylightHours()
{
	time_t currTime = time(NULL);
	struct tm *currLocalTime = localtime(&currTime);

	int hour = currLocalTime->tm_hour;
	// Note this is strictly less than 6 and strictly greater than 18, so it turns on at 6am off at 7pm
	if ((hour < 6) || (hour > 18)) {
		// It's night-time
		return false;
	}
	else {
		// It's day-time
		return true;
	}
}

void BehavioralBoxLabjack::updateVisibleLightRelayIfNeeded()
{
	bool isDay = isArtificialDaylightHours();
	this->setVisibleLightRelayState(isDay);
}