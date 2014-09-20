//
// Packet definitions for realtime data from Grip.
//

// The port number used to access EPM servers.
// EPM-OHB-SP-0005 says:
//  The Port number for all EPM LAN connections is 2345.

#define EPM_DEFAULT_PORT "2345"

// Per EPM-OHB-SP-0005, packets shall not exceed 1412 octets.
#define EPM_BUFFER_LENGTH	1412	

// Telemetry packets have the following sync marker.
#define EPM_TELEMETRY_SYNC_VALUE	0xFFDB544D
#define GRIP_HK_ID	0x0301
#define GRIP_RT_ID	0x1001

typedef struct {

	unsigned long  epmSyncMarker;
	unsigned char  subsystemMode;
	unsigned char  subsystemID;
	unsigned char  destination;
	unsigned char  subsystemUnitID;
	unsigned short TMIdentifier;
	unsigned short TMCounter;
	unsigned char  model;
	unsigned char  taskID;
	unsigned short subsystemUnitVersion;
	unsigned short coarseTimeHigh;
	unsigned short coarseTimeLow;
	unsigned short fineTime;
	unsigned char  timerStatus;
	unsigned char  experimentMode;
	unsigned short checkSumIndicator;
	unsigned char  receiverSubsystemID;
	unsigned char  receiverSubsystemUnitID;
	unsigned short numberOfWords;

} EPMTelemetryHeader;

typedef union {
	// A buffer containing the entire packet.
	char				buffer[EPM_BUFFER_LENGTH];
	// Allows easy acces to the header elements by name.
	EPMTelemetryHeader	header;
} EPMTelemetryPacket;

// Define a static packet header that is representative of a housekeeping packet.
// We don't try to simulate all the details, so most of the parameters are set to zero.
// The TM Identifier is 0x0301 for DATA_BULK_HK per DEX-ICD-00383-QS.
// The total number of words is 114 / 2 = 57 for the GRIP packet, 15 for the EPM header and 1 for the checksum.
static EPMTelemetryHeader hkHeader = { EPM_TELEMETRY_SYNC_VALUE, 0, 0, 0, 0, GRIP_HK_ID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 73 };
static int hkPacketLengthInBytes = 146;

// Do the same for a realtime data packet.
// The TM Identifier is 0x1001 for DATA_RT_SCIENCE per DEX-ICD-00383-QS.
// The total number of words is 758 / 2 = 379 for the GRIP packet, 15 for the EPM header and 1 for the checksum.
static EPMTelemetryHeader rtHeader = { EPM_TELEMETRY_SYNC_VALUE, 0, 0, 0, 0, GRIP_RT_ID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 773 };
static int rtPacketLengthInBytes = 790;
