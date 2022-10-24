#include "RoomServer.h"

#include "../Common/H/common.h"
#include "../Common/Proto/client_listener.pb.h"
#include "Listener.h"

CRoomServer::CRoomServer()
    : CWebSocketServer()
{
}

CRoomServer::~CRoomServer()
{
    CloseHandle(mRoomProcessInfo.pi.hProcess);
    CloseHandle(mRoomProcessInfo.pi.hThread);
}

void CRoomServer::setClientsListener(CListener* listener)
{
    mListener = listener;
}

void CRoomServer::getGameStatusInfo(RoomInfo& roomInfo)
{
    roomInfo = mRoomInfo;
}

bool CRoomServer::run(wchar_t* folder, unsigned short port)
{
    t = new std::thread(&CRoomServer::process, this, port);
    return true;
}

void CRoomServer::sendInvitationToViewGame(int16_t port, std::string host,
    int32_t ticket)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    ServerWrappedMessage swm;
    InvitationToViewGame* invitation = new InvitationToViewGame();
    invitation->set_clientip(host);
    invitation->set_clientport(port);
    invitation->set_ticket(ticket);
    swm.set_allocated_invitationtoview(invitation);

    std::ostringstream out;
    swm.SerializeToOstream(&out);

    send(mRoomConnection, out.str(), websocketpp::frame::opcode::binary);
}

void CRoomServer::sendInvitationToPlayGame(int16_t port, std::string host, int32_t ticket)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    ServerWrappedMessage swm;
    InvitationToPlayGame* invitation = new InvitationToPlayGame();
    invitation->set_clientip(host);
    invitation->set_clientport(port);
    invitation->set_ticket(ticket);
    swm.set_allocated_invitationtoplay(invitation);

    std::ostringstream out;
    swm.SerializeToOstream(&out);

    send(mRoomConnection, out.str(), websocketpp::frame::opcode::binary);
}

void CRoomServer::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    log(elfRoom, "message from the room");
    std::istringstream in(msg->get_payload());
    RoomWrappedMessage rwm;
    rwm.ParseFromIstream(&in);
    if (rwm.has_roominfo())
    {
        mRoomInfo.set_roomname(rwm.roominfo().roomname());
        mRoomInfo.set_gamestatus(rwm.roominfo().gamestatus());
        mRoomInfo.set_roomsize(rwm.roominfo().roomsize());
        mRoomInfo.set_waitingsize(rwm.roominfo().waitingsize());
        log(elfRoom, std::string("status = ") + std::to_string(mRoomInfo.gamestatus()) +
            std::string("; room size = ") + std::to_string(mRoomInfo.roomsize()) +
            std::string("; waiting size = ") + std::to_string(mRoomInfo.waitingsize()));
    }
    else if (rwm.has_responceinvitation())
    {
        auto conn = get_con_from_hdl(hdl);
        std::string remote = conn->get_remote_endpoint();
        auto responce = rwm.responceinvitation();
        mListener->responceOnInvitationToGame(responce);
        log(elfRoom, "responce about invitation to game; responce = " + std::to_string(responce.responce()) +
            "; ticket = " + std::to_string(responce.ticket()) + "; host = " + responce.clientip() +
            "; port = " + std::to_string(responce.clientport()));
    }
    else if (rwm.has_responceview())
    {
        auto conn = get_con_from_hdl(hdl);
        std::string remote = conn->get_remote_endpoint();
        auto responce = rwm.responceview();
        mListener->responceOnViewGame(responce);
        log(elfRoom, "responce about view the to game; responce = " + std::to_string(responce.responce()) +
            "; host = " + responce.clientip() + "; port = " + std::to_string(responce.clientport()));
    }
    else if (rwm.has_startgame())
    {
        auto ticket = rwm.startgame().ticket();
        log(elfRoom, "New game with ticket = " + std::to_string(ticket) + " is started");
    }
    else
    {
        log(elfRoom, "unrecognized message");
        return;
    }

    // send info about this room to the all clients (update info)
    auto conn = get_con_from_hdl(hdl);
    mListener->sendAllInfoAboutTheGame(hdl, conn->get_port());
}

void CRoomServer::on_close_handler(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);
    auto conn = get_con_from_hdl(hdl);
    auto port = conn->get_port();
    auto host = conn->get_host();
    log(elfRoom, "close connection to room with ip = " + host + " and port = " + std::to_string(port));

    if (!hdl.owner_before(mRoomConnection) && !mRoomConnection.owner_before((hdl)))
    {
        // find the appropriate room and clear all its parameters
        mRoomInfo.set_gamestatus(RoomInfo_GameStatusEnum_gseNone);
        mRoomInfo.set_roomsize(0);
        mRoomInfo.set_waitingsize(0);
        mListener->sendAllInfoAboutTheGame(hdl, conn->get_port());
        mRoomConnection.reset();
    }
}

void CRoomServer::on_open_handler(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    // this handler is allowed only for one client
    auto uc = mRoomConnection.use_count();
    if (uc != 0)
    {
        log(elfRoom, "there is established connection already");
        close(hdl, 0, "");
        return;
    }

    mRoomConnection = hdl;

    // check if another side is a room
    auto conn = get_con_from_hdl(hdl);
    //auto endPoint = conn->get_remote_endpoint();
    auto port = conn->get_port();
    auto host = conn->get_host();

    auto rep = conn->get_remote_endpoint();

    boost::system::error_code ec;
    auto localEndpoint = conn->get_raw_socket().local_endpoint();
    auto remoteEndpoint = conn->get_remote_endpoint();

    std::cout << "error code = " << ec.message() << "; local port = " << localEndpoint.port() <<
        "; local address " << localEndpoint.address() << "; remote = " << remoteEndpoint << std::endl;

    if (host != "localhost")
    {
        log(elfRoom, "connect to room with ip = " + host + " and port = " + std::to_string(port) + " is prohibited");
        close(hdl, 0, "");
        return;
    }
    else
    {
        log(elfRoom, "connect to room with ip = " + host + " and port = " + std::to_string(port));
    }
}
