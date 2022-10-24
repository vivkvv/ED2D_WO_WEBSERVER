#include "PlayersCommunicator.h"

#include "../Common/H/common.h"
#include "GameRoom.h"

CPlayersCommunicator::CPlayersCommunicator(CGameRoom* gameRoom)
    : CWebSocketServer()
    , mGameRoom(gameRoom)
{
}

CPlayersCommunicator::~CPlayersCommunicator()
{
}

void CPlayersCommunicator::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
    log(elfClient, "on client's message");
    mGameRoom->onMessage(hdl, msg);
}

void CPlayersCommunicator::on_close_handler(websocketpp::connection_hdl hdl)
{
    log(elfClient, "on client's close handler");
    mGameRoom->onCloseHandler(hdl);
}

void CPlayersCommunicator::on_open_handler(websocketpp::connection_hdl hdl)
{
    log(elfClient, "on client's open handler");

    auto conn = get_con_from_hdl(hdl);
    auto& socket = conn->get_raw_socket();
    boost::asio::ip::tcp::no_delay option(true);
    socket.set_option(option);

    if (!mGameRoom->onOpenHandler(hdl))
    {
        log(elfClient, "unexpected client");
        // don't close connection
        // maybe, it is viewer
        //close(hdl, 0, "");
    }
}

void CPlayersCommunicator::getSelfAddressAndPort(std::string& address, uint16_t& port)
{
    address = "127.0.0.1";
    port = getPort();
}
