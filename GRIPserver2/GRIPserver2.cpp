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

			if ( sendHK ) {
				// Insert the current packet count into the packet.
				hkPacket.header.TMCounter = packetCount++;
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

			// Insert the current packet count into the packet.
			rtPacket.header.TMCounter = packetCount++;
			// Send out a realtime data packet.
			iSendResult = send( ClientSocket, rtPacket.buffer, rtPacketLengthInBytes, 0 );
			// If we get a socket error it is probably because the client has closed the connection.
			// So we break out of the loop.
			if (iSendResult == SOCKET_ERROR) {
				fprintf( stderr, "RT packet send failed with error: %3d\n", WSAGetLastError());
				break;
			}
			fprintf( stderr, "  RT packet %3d Bytes sent: %3d\n", packetCount, iSendResult);

			// RT packets get sent out by GRIP twice per second.
			Sleep( 500 );

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

