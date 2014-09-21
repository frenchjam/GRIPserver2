//
// GRIPserver2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GripPackets.h"

PCSTR EPMport = EPM_DEFAULT_PORT;
EPMTelemetryPacket hkPacket, rtPacket;

#ifdef _DEBUG
	bool _debug = true;
#else
	bool _debug = false;
#endif

void setPacketTime( EPMTelemetryPacket *packet ) {

	// Set the time of the specified packet.

	// Time structure to get the local time for the EPM packet coarse and fine time values.
	// I am using the 32 bit version because the EPM coarse time is 32 bits.
	struct __timeb32 epmtime;
	_ftime32_s( &epmtime );

	// NB EPM uses GPS time (second since midnight Jan 5-6 1980), while 
	// _ftime_s() uses seconds since midnight Jan. 1 1970 UTC.
	// I am just using UTC time, since it doesn't really matter for 
	// my purposes here. The idea is just to keep the packets in the right order.
	// Also, EPM somehow gets time in 10ths of milliseconds and puts that in the header. 
	// We don't expect to get two packet in a span of less than a millisecond, so I don't worry about it.

	// One could probably treat coarseTime as an unsigned long in the
	// header and just copy the 32-bit value, but I'm not sure about word order. 
	// So to be sure to match the EPM spec as it is written, I transfer each
	//  two-bye word separately.
	packet->header.coarseTimeLow = (short)(epmtime.time & 0x0000ffff);
	packet->header.coarseTimeHigh = (short) ((epmtime.time & 0xffff0000) << 16);
	// Compute the fine time in 10ths of milliseconds.
	packet->header.fineTime = epmtime.millitm * 10;

}

int _tmain(int argc, char *argv[])
{
	WSADATA wsaData;
    int iResult;

	int packetCount = 0;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
	int sendHK = 1;
    	
	if ( _debug ) fprintf( stderr, "%s started.\n\n", argv[0] );
	fprintf( stderr, "This is the EPM/GRIP packet server emulator.\n" );
	fprintf( stderr, "It waits for a client to connect and then sends\n out HK and RT packets at 0.5 and 1 Hz respectively.\n" );
	fprintf( stderr, "\n" );

	// Prepare the packets.
	memcpy( &hkPacket, &hkHeader, sizeof( hkHeader ) );
	memcpy( &rtPacket, &rtHeader, sizeof( rtHeader ) );

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        fprintf( stderr, "WSAStartup failed with error: %d\n", iResult );
        return 1;
    }
	else if ( _debug ) fprintf( stderr, "WSAStartup() OK.\n" );

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, EPMport, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 2;
    }
	else if ( _debug ) fprintf( stderr, "getaddrinfo() OK.\n" );

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 3;
    }
	else if ( _debug ) fprintf( stderr, "ListenSocket() OK.\n" );

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 4;
    }
	else if ( _debug ) fprintf( stderr, "bind() OK.\n" );

	// We don't need the address info anymore, so free it.
    freeaddrinfo(result);

	// Enter an infinite loop that listens for connections,
	//  outputs packets as long as the connection is valid and
	//  then exits. 
	// The only way out is to kill the program (<ctrl-c>).
	// NB We effectively only allow one client at a time.

	while ( 1 ) {

		// Listen until we get a connection.
		fprintf( stderr, "Listening for a connection ... " );
		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 5;
		}
		else if ( _debug ) fprintf( stderr, "listen() OK " );

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			fprintf( stderr, "accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 6;
		}
		else if ( _debug ) fprintf( stderr, "acceot() OK " );
		fprintf( stderr, "connected.\n" );

		// Send packets until the peer shuts down the connection
		while ( 1 ) {

			// RT packets get sent out by GRIP twice per second.
			// This is a trick to avoid drift in the rate.
			// We compute the number of milliseconds to sleep to get back to a 500 ms boundary.
			Sleep( 10 );	
			struct __timeb32 utctime;
			_ftime32_s( &utctime );
			Sleep( (1000 - utctime.millitm ) % 500 );

			// Insert the current packet count and time into the packet.
			rtPacket.header.TMCounter = packetCount++;
			setPacketTime( &rtPacket );
			// Send out a realtime data packet.
			iSendResult = send( ClientSocket, rtPacket.buffer, rtPacketLengthInBytes, 0 );
			// If we get a socket error it is probably because the client has closed the connection.
			// So we break out of the loop.
			if (iSendResult == SOCKET_ERROR) {
				fprintf( stderr, "RT packet send failed with error: %3d\n", WSAGetLastError());
				break;
			}
			fprintf( stderr, "  RT packet %3d Bytes sent: %3d\n", packetCount, iSendResult);

			// HK packets get sent once every 2 seconds. 
			// The BOOL sendHK is used to turn off and on for each RT cycle.
			if ( sendHK ) {
				// Insert the current packet count and time into the packet.
				hkPacket.header.TMCounter = packetCount++;
				setPacketTime( &hkPacket );
				// Send out a housekeeping packet.
				iSendResult = send( ClientSocket, hkPacket.buffer, hkPacketLengthInBytes, 0 );
				// If we get a socket error it is probably because the client has closed the connection.
				// So we break out of the loop.
				if (iSendResult == SOCKET_ERROR) {
					fprintf( stderr, "HK send failed with error: %3d\n", WSAGetLastError());
					break;
				}
				fprintf( stderr, "  HK packet %3d Bytes sent: %3d\n", packetCount, iSendResult);
			}
			sendHK = !sendHK; // Toggle enable flag so that we do one out of two cycles.

			// Every once in a while, pause a bit to simulate breaks between tasks.
			if ( (packetCount % 20) == 0 ) {
				fprintf( stderr, "\nSimulating inter-trial pause.\n\n" );
				Sleep( 5000 );
			}

		} while (iResult > 0);

		// shutdown the connection since we're done
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			fprintf( stderr, "shutdown() failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 7;
		}
		else if ( _debug ) fprintf( stderr, "shutdown() OK n" );

		fprintf( stderr, "  Total packets sent: %d\n\n", hkPacket.header.TMCounter + rtPacket.header.TMCounter );
		hkPacket.header.TMCounter = 0;
		rtPacket.header.TMCounter = 0;

	}

    // cleanup
    closesocket(ListenSocket);
    closesocket(ClientSocket);
    WSACleanup();

	return 0;
}

