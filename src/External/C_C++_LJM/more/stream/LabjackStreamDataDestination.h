#pragma once
#include <stdio.h>
#include <string.h>

class LabjackStreamDataDestination
{
public:
	LabjackStreamDataDestination();
	~LabjackStreamDataDestination();
	
	void accumulateScans(int numScans, int numChannels, const char** channelNames, const int* channelAddresses, double* aData);

private:
	int numOfTotalScans = 0;
	int numChannels = 0;
	
};

