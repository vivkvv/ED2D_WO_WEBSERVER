#pragma once

#include "../Common/H/WebSocketClient.h"

class CGameRoom;

class CServerCommunicator : public CWebSocketClient
{
public:
    CServerCommunicator(CGameRoom* gameRoom);
    ~CServerCommunicator();

    void run(uint16_t port);

    websocketpp::connection_hdl toServerConnection();
    int getTicket();

    //HANDLE startEvent = nullptr;

private:
    std::mutex mMutex;

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg);
    void on_close_handler(websocketpp::connection_hdl hdl);
    void on_open_handler(websocketpp::connection_hdl hdl);

    CGameRoom* mGameRoom;
    std::unique_ptr<std::thread> mThread = nullptr;

    websocketpp::connection_hdl mConnection;
    int mTicket = -1;
};

