// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// In Microsoft Visual C++ Express, the property:
//	Configuration Properties - General - Project Defaults - Character Set 
// is set to "Not Set" to stick with regular, old-school stings.

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <SYS\timeb.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")
