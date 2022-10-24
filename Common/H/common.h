#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#define DEFAULT_BUFLEN 512

enum ELogFrom { elfSelf, elfClient, elfListener, elfRoom };

void log(ELogFrom from, std::string logString);

/*
enum GameStatusEnum { gseNone, gseWaiting, gsePlaying };

struct SGameStatusInfo
{
    GameStatusEnum mGameStatus = gseNone;
    size_t mRoomSize = -1;
    size_t mWaitingSize = -1;
};
*/
