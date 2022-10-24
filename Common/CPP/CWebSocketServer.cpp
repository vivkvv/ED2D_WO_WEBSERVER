#include "../H/CWebSocketServer.h"
#include "../H/common.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef CWebSocketServer::message_ptr message_ptr;

CWebSocketServer::CWebSocketServer() :
    websocketpp::server<websocketpp::config::asio>()
{}


void CWebSocketServer::on_fail_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "server fail");
}

void CWebSocketServer::on_pong_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "server pong");
}

void CWebSocketServer::on_pong_timeout_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "server pong timeout");
}

void CWebSocketServer::on_interrupt_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "server interrupt");
}

void CWebSocketServer::on_http_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "server http");
}

unsigned short CWebSocketServer::getPort()
{
    return mPort;
}

void CWebSocketServer::process(unsigned short port)
{
    try
    {
        mPort = port;

        // Set logging settings
        set_access_channels(websocketpp::log::alevel::all);
        clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        init_asio();

        // Register our message handler
        set_message_handler(bind(&CWebSocketServer::on_message, this, ::_1, ::_2));
        set_close_handler(bind(&CWebSocketServer::on_close_handler, this, ::_1));
        set_open_handler(bind(&CWebSocketServer::on_open_handler, this, ::_1));

        set_fail_handler(bind(&CWebSocketServer::on_fail_handler, this, ::_1));
        //set_ping_handler(bind(&CWebSocketServer::on_ping_handler, this, ::_1, ::_2));
        set_pong_handler(bind(&CWebSocketServer::on_pong_handler, this, ::_1));
        set_pong_timeout_handler(bind(&CWebSocketServer::on_pong_timeout_handler, this, ::_1));
        set_interrupt_handler(bind(&CWebSocketServer::on_interrupt_handler, this, ::_1));
        set_http_handler(bind(&CWebSocketServer::on_http_handler, this, ::_1));
        //set_validate_handler(bind(&CWebSocketClient::on_validate_handler, this, ::_1));

        // Listen on port
        listen(port);

        // Start the server accept loop
        start_accept();

        // Start the ASIO io_service run loop
        run();
    }
    catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "other exception" << std::endl;
    }

    log(elfSelf, "thread ending");
}

