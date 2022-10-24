#pragma once

#include "../Common/H/CWebSocketServer.h"

class CGameRoom;

class CPlayersCommunicator :    public CWebSocketServer
{
public:
    CPlayersCommunicator(CGameRoom* gameRoom);
    ~CPlayersCommunicator();

    void getSelfAddressAndPort(std::string& address, uint16_t& port);

private:
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) override;
    void on_close_handler(websocketpp::connection_hdl hdl) override;
    void on_open_handler(websocketpp::connection_hdl hdl) override;

    CGameRoom* mGameRoom;
};

