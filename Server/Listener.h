#pragma once

#include "../Common/H/CWebSocketServer.h"
#include "../Common/Proto/client_listener.pb.h"

#include <map>

class CRoomsContainer;

//----------------------------

struct WebRTCConnectionInfo
{
    //websocketpp::connection_hdl playerHdl;
    int intConnectionHandle;
    websocketpp::connection_hdl connection_hdl;
    WebRTCPlayerStatus playerStatus;
};

struct WebRTCGames
{
    std::string mGameName;
    int32_t roomSize;
    std::vector<WebRTCConnectionInfo> mRTCConnections;
};

//----------------------------

class CListener : public CWebSocketServer
{
public:
    CListener(CRoomsContainer* roomContainer);
    ~CListener();

    void responceOnInvitationToGame(ResponceOnGameInvitation& responce);
    void responceOnViewGame(ResponceOnGameView& responce);

    void sendAllInfoAboutTheGame(websocketpp::connection_hdl hdl, unsigned short port);

private:
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) override;
    void on_close_handler(websocketpp::connection_hdl hdl) override;
    void on_open_handler(websocketpp::connection_hdl hdl) override;

    static int getIntFromHandle(websocketpp::connection_hdl hdl);

    int32_t generateNewTicket();

    int32_t mCurrentTicket = 0;

    std::mutex mMutex;
    std::map<int, websocketpp::connection_hdl> mPlayConnections;
    std::map<int, websocketpp::connection_hdl> mViewersConnections;
    std::map<int, websocketpp::connection_hdl> mSubscribersConnections;

    CRoomsContainer* mRoomContainer = nullptr;
    std::map<int, WebRTCGames> webRTCGames;

    void sendAllInfoAboutAllTheGames(websocketpp::connection_hdl hdl);

    void sendAllInfoAboutTheRTCGame(int32_t ticket);
    void sendAllInfoAboutTheRTCGames(websocketpp::connection_hdl hdl);
    void sendRefuseAboutRTCGame(websocketpp::connection_hdl hdl, int32_t ticket,
        WebRTCGameRefuseReasons reason);
    void sendOfferToWebRTCClient(websocketpp::connection_hdl hdl, int32_t ticket,
        std::string offerString);
    void sendAnswerToWebRTCAdmin(websocketpp::connection_hdl hdl, int32_t ticket,
        std::string answerString);
};

