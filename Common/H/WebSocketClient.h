#pragma once

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

class CWebSocketClient : public websocketpp::client<websocketpp::config::asio_client>
{
public:
    CWebSocketClient();

    void process(uint16_t port);

    virtual void on_message(websocketpp::connection_hdl hdl, message_ptr msg) = 0;
    virtual void on_close_handler(websocketpp::connection_hdl hdl) = 0;
    virtual void on_open_handler(websocketpp::connection_hdl hdl) = 0;

    virtual void on_fail_handler(websocketpp::connection_hdl hdl);
    //virtual void on_ping_handler(websocketpp::connection_hdl hdl, std::string str);
    virtual void on_pong_handler(websocketpp::connection_hdl hdl);
    virtual void on_pong_timeout_handler(websocketpp::connection_hdl hdl);
    virtual void on_interrupt_handler(websocketpp::connection_hdl hdl);
    virtual void on_http_handler(websocketpp::connection_hdl hdl);
    //virtual void on_validate_handler(websocketpp::connection_hdl hdl);

};
