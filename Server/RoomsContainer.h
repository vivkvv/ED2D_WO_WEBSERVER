#pragma once

#include <vector>
#include "RoomServer.h"

class CListener;

class CRoomsContainer
{
public:
    CRoomsContainer(size_t size);
    ~CRoomsContainer();

    bool run(wchar_t* folder, unsigned short startPort);
    bool passPlayersToRoom(int16_t port, std::string host, int32_t ticket,
        std::string roomIp, int32_t roomPort);
    bool passViewerToRoom(int16_t port, std::string host, int32_t ticket,
        std::string roomIp, int32_t roomPort);
    void setClientsListener(CListener* listener);
    void getAllInfoAboutAllTheGames(std::vector<GameState>& gamesState);
    bool getAllInfoAboutTheGame(unsigned short port, RoomInfo& roomInfo);

private:
    std::mutex mMutex;
    std::vector<std::unique_ptr<CRoomServer>> mRooms;
};

