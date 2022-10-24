#include "RoomsContainer.h"

#include "../Common/H/common.h"

CRoomsContainer::CRoomsContainer(size_t size)
{
    std::unique_lock<std::mutex> mLocker(mMutex);
    mRooms.resize(size);

    for (size_t i = 0; i < size; ++i)
    {
        mRooms[i] = std::make_unique<CRoomServer>();
    }
}

void CRoomsContainer::setClientsListener(CListener* listener)
{
    for (size_t i = 0; i < mRooms.size(); ++i)
    {
        mRooms[i]->setClientsListener(listener);
    }
}

bool CRoomsContainer::getAllInfoAboutTheGame(unsigned short port, RoomInfo& roomInfo)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    for (int i = 0; i < mRooms.size(); ++i)
    {
        auto& room = mRooms[i];

        if (room->getPort() == port)
        {
            room->getGameStatusInfo(roomInfo);
            return true;
        }
    }

    return false;
}

void CRoomsContainer::getAllInfoAboutAllTheGames(std::vector<GameState>& gamesState)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    for (int i = 0; i < mRooms.size(); ++i)
    {
        auto& room = mRooms[i];

        RoomInfo* roomInfo = new RoomInfo;
        room->getGameStatusInfo(*roomInfo);

        GameState gs;
        gs.set_roomport(room->getPort());
        gs.set_allocated_roominfo(roomInfo);

        gamesState.push_back(gs);
    }
}

bool CRoomsContainer::passViewerToRoom(int16_t port, std::string host, int32_t ticket,
    std::string roomIp, int32_t roomPort)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    for (int i = 0; i < mRooms.size(); ++i)
    {
        auto& room = mRooms[i];
        RoomInfo roomInfo;
        room->getGameStatusInfo(roomInfo);
        if (room->getPort() == roomPort &&
            roomInfo.gamestatus() == RoomInfo_GameStatusEnum_gsePlaying)
        {
            room->sendInvitationToViewGame(port, host, ticket);
            log(elfSelf, "new view query has been passed to a room");
            return true;
        }
    }

    log(elfSelf, "Can't find an apropriate room for viewing");
    return false;
}

bool CRoomsContainer::passPlayersToRoom(int16_t port, std::string host, int32_t ticket,
    std::string roomIp, int32_t roomPort)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    for (int i = 0; i < mRooms.size(); ++i)
    {
        auto& room = mRooms[i];
        RoomInfo roomInfo;
        room->getGameStatusInfo(roomInfo);
        if (room->getPort() == roomPort &&
            roomInfo.gamestatus() == RoomInfo_GameStatusEnum_gseWaiting &&
            roomInfo.waitingsize() > 0)
        {
            room->sendInvitationToPlayGame(port, host, ticket);
            log(elfSelf, "new play query has been passed to a room");
            return true;
        }
    }

    log(elfSelf, "There is no free room");
    return false;
}

bool CRoomsContainer::run(wchar_t* folder, unsigned short startPort)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    for (size_t i = 0; i < mRooms.size(); ++i)
    {
        unsigned short port = startPort + i;
        std::wstring startName = L"START_ROOM_EVENT_" + std::to_wstring(port);
        bool result = mRooms[i]->run(folder, port);

        if (!result)
        {
            log(elfSelf, "can't run room");
            return false;
        }

    }

    return true;
}


CRoomsContainer::~CRoomsContainer()
{
    std::unique_lock<std::mutex> mLocker(mMutex);
    mRooms.resize(0);
}
