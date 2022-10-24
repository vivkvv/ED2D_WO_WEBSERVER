#pragma once

#include "../H/WebSocketClient.h"
#include "../H/common.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

CWebSocketClient::CWebSocketClient()
    : websocketpp::client<websocketpp::config::asio_client>()
{

}

void CWebSocketClient::on_fail_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "client fail");
}

void CWebSocketClient::on_pong_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "client pong");
}

void CWebSocketClient::on_pong_timeout_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "client pong timeout");
}

void CWebSocketClient::on_interrupt_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "client interrupt");
}

void CWebSocketClient::on_http_handler(websocketpp::connection_hdl hdl)
{
    log(elfSelf, "client http");
}

void CWebSocketClient::process(uint16_t port)
{
    try
    {
        std::string uri = "ws://localhost:" + std::to_string(port);

        set_access_channels(websocketpp::log::alevel::all);
        clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize ASIO
        init_asio();

        // Register our message handler
        set_message_handler(bind(&CWebSocketClient::on_message, this, ::_1, ::_2));
        set_close_handler(bind(&CWebSocketClient::on_close_handler, this, ::_1));
        set_open_handler(bind(&CWebSocketClient::on_open_handler, this, ::_1));

        set_fail_handler(bind(&CWebSocketClient::on_fail_handler, this, ::_1));
        //set_ping_handler(bind(&CWebSocketClient::on_ping_handler, this, ::_1, ::_2));
        set_pong_handler(bind(&CWebSocketClient::on_pong_handler, this, ::_1));
        set_pong_timeout_handler(bind(&CWebSocketClient::on_pong_timeout_handler, this, ::_1));
        set_interrupt_handler(bind(&CWebSocketClient::on_interrupt_handler, this, ::_1));
        set_http_handler(bind(&CWebSocketClient::on_http_handler, this, ::_1));
        //set_validate_handler(bind(&CWebSocketClient::on_validate_handler, this, ::_1));


        websocketpp::lib::error_code ec;
        client::connection_ptr con = get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return;
        }

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        connect(con);

        // Start the ASIO io_service run loop
        // this will cause a single connection to be made to the server. c.run()
        // will exit when this connection is closed.
        run();
    }
    catch (websocketpp::exception const & e)
    {
        std::cout << e.what() << std::endl;
    }

    log(elfSelf, "client thread ends");
}

