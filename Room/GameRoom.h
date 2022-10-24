#pragma once

#include "PlayersCommunicator.h"
#include "../Common/H/common.h"
#include "../Common/Proto/client_listener.pb.h"
#include "Charge2D.h"
#include "toJS.h"
#include <chrono>

#include <map>
#include <thread>

class CServerCommunicator;

typedef CWebSocketServer::message_ptr message_ptr;

typedef std::map<int, CCharge2D> TCharges2DMap;

class CGameRoom
{
public:
    CGameRoom(const std::string& name, int width, int height, int players,
        ShowStruct forcesShowStruct, ShowStruct eqPotentialsShowStruct,
        const std::vector<CCharge2D>& charges2D);
    virtual ~CGameRoom();

    void start(int port);

    void setServerCommunicator(CServerCommunicator* server);
    void runGame();
    void stopGame();

    void sendReplyOnGameInvitation(websocketpp::connection_hdl hdl, int32_t ticket);
    void sendReplayOnGameView(websocketpp::connection_hdl hdl, int32_t ticket);
    void sendRoomStatusToServer(websocketpp::connection_hdl hdl);

    void sendStartGameInfoToServer(websocketpp::connection_hdl hdl);

    void onCloseHandler(websocketpp::connection_hdl hdl);
    bool onOpenHandler(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, message_ptr msg);

    void sendStartConditionsToAllPlayers();
    void sendAllParametersToAllPlayers();
    void calculateNewParameters(std::chrono::milliseconds& currentTime);

private:
    std::vector<CCharge2D> mXmlConfigurationCharges2D;

    ShowStruct mForcesShowStruct;
    ShowStruct mEqPotentialsShowStruct;

    float mQulon = 0.5;
    //const float epsilon = 5.0;
    //const float mu = 1.0;
    float mC = 2.0;
    //float mC2 = mC * mC;
    bool mBCalculated = false;

    bool mGameStop = false;
    int mNextId = 0;
    int mPacketId = 0;
    float mWidth = 800.0;
    float mHeight = 600.0;

    std::mutex mMutex;
    CPlayersCommunicator mPlayersCommunicator;
    std::unique_ptr<std::thread> mThread = nullptr;
    CServerCommunicator* mServerCommunicator = nullptr;

    RoomInfo mRoomInfo;

    struct TPlayerData
    {
        int id;
    };
    typedef std::map<websocketpp::connection_hdl, TPlayerData,
        std::owner_less<websocketpp::connection_hdl>> con_list;
    con_list mPlayersConnections;

    struct TViewerData
    {
        bool enabled;
    };
    typedef std::map<websocketpp::connection_hdl, TViewerData,
        std::owner_less<websocketpp::connection_hdl>> view_list;
    view_list mViewersConnections;

    TCharges2DMap mCharges2DMap;

    void run(int port);

    int getNextId();
    int getNextPacketId();

    void initializeGame();
    void closeAllConnections();

    template <class T>
    void sendAllParametersToPlayerWithConnection(
        const T& connection,
        int32_t packetId);
};

