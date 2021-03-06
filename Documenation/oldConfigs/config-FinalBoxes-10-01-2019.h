#pragma once
//config-UpstairsBox-8-20-2019.h:
/*
Specifies what ports to open. Imported by config.h.
*/


//// Inputs:
// Digital Breakout Board:
enum { NUM_CHANNELS = 9 };
#define globalLabjackInputPortNames { "EIO0","EIO1","EIO2","EIO3","EIO4","EIO5","EIO6","EIO7","CIO0"}
#define globalLabjackInputPortPurpose { "Water1_BeamBreak","Water2_BeamBreak","Food1_BeamBreak","Food2_BeamBreak","Water1_Dispense","Water2_Dispense","Food1_Dispense","Food2_Dispense","RunningWheel_Beambreak"}

//// Outputs:
enum { NUM_OUTPUT_CHANNELS = 1 };
#define globalLabjackOutputPortNames { "CIO1" }
#define globalLabjackOutputPortPurpose { "VisibleLedRelay" }

// File Locations:
#define globalOutputFileDirectory "C:/Common/data/"
#define globalHistoricalFileSearchDirectory globalOutputFileDirectory
