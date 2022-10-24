#include "ServerCommunicator.h"

#include "../Common/H/common.h"
#include "GameRoom.h"

CServerCommunicator::CServerCommunicator(CGameRoom* gameRoom)
    : CWebSocketClient()
    , mGameRoom(gameRoom)
{
    mGameRoom->setServerCommunicator(this);

    /*
    startEvent = CreateEvent(
        NULL,               // default security attributes
        FALSE,              // manual-reset event
        FALSE,              // initial state is nonsignaled
        L""
    );
    */

}

CServerCommunicator::~CServerCommunicator()
{
    //CloseHandle(startEvent);
}

websocketpp::connection_hdl CServerCommunicator::toServerConnection()
{
    return mConnection;
}

int CServerCommunicator::getTicket()
{
    return mTicket;
}

void CServerCommunicator::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
    log(elfListener, "message from the room communicator");

    std::istringstream in(msg->get_payload());
    ServerWrappedMessage swm;
    swm.ParseFromIstream(&in);
    if (swm.has_invitationtoplay())
    {
        // check, if there is enough room
        // and send responce
        mTicket = swm.invitationtoplay().ticket();
        mGameRoom->sendReplyOnGameInvitation(hdl, mTicket);
    }
    else if (swm.has_invitationtoview())
    {
        auto ticket = swm.invitationtoview().ticket();
        mGameRoom->sendReplayOnGameView(hdl, ticket);
    }
    else
    {
        log(elfListener, "unrecognized message");
    }

}

void CServerCommunicator::run(uint16_t port)
{
    mThread = std::make_unique<std::thread>(&CServerCommunicator::process, this, port);
}

void CServerCommunicator::on_close_handler(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    mConnection.reset();

    auto conn = get_con_from_hdl(hdl);
    auto port = conn->get_port();
    auto host = conn->get_host();
    log(elfListener, "close connection to server with ip = " + host + " and port = " + std::to_string(port));
}

void CServerCommunicator::on_open_handler(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    if (mConnection.use_count() != 0)
    {
        log(elfListener, "there is established connection already");
        close(hdl, 0, "");
        return;
    }

    mConnection = hdl;

    boost::system::error_code ec;
    auto conn = get_con_from_hdl(hdl);
    auto localEndpoint = conn->get_raw_socket().local_endpoint();
    auto remoteEndpoint = conn->get_remote_endpoint();
    auto host = conn->get_host();
    auto port = conn->get_port();

    std::cout << "error code = " << ec.message() << "; local port = " << localEndpoint.port() <<
        "; local address " << localEndpoint.address() << "; remote = " << remoteEndpoint << std::endl;

    // one local connection
    if (host != "localhost")
    {
        log(elfListener, "connect to room with ip = " + host + " and port = " + std::to_string(port) + " is prohibited");
        close(hdl, 0, "");
        return;
    }
    else
    {
        log(elfListener, "connect to room with ip = " + host + " and port = " + std::to_string(port));
    }

    //SetEvent(startEvent);

    mGameRoom->sendRoomStatusToServer(hdl);
}

