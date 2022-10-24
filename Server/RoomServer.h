#pragma once

#include "../Common/H/CWebSocketServer.h"
#include "../Common/H/common.h"
#include "../Common/Proto/client_listener.pb.h"

class CListener;

class CRoomServer : public CWebSocketServer
{
public:
    CRoomServer();
    ~CRoomServer();

    void setClientsListener(CListener* listener);

    bool run(wchar_t* folder, unsigned short port);
    void getGameStatusInfo(RoomInfo& roomInfo);
    void sendInvitationToPlayGame(int16_t port, std::string host, int32_t ticket);
    void sendInvitationToViewGame(int16_t port, std::string host, int32_t ticket);

private:
    std::thread* t = nullptr;

    CListener* mListener = nullptr;

    std::mutex mMutex;
    websocketpp::connection_hdl mRoomConnection;

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) override;
    void on_close_handler(websocketpp::connection_hdl hdl) override;
    void on_open_handler(websocketpp::connection_hdl hdl) override;

    struct RoomProcessInfo
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
    } mRoomProcessInfo;

    RoomInfo mRoomInfo;
};

