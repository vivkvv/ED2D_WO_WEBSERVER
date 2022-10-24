// Server.cpp : Defines the entry point for the console application.
//

#include <winsock2.h>
#include <ws2tcpip.h>

//#include <pathcch.h>
//#pragma comment(lib, "Pathcch.lib")

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include <string>
#include <vector>

#include "Listener.h"
#include "RoomsContainer.h"
#include "../Common/H/common.h"

bool implicitPassSelfIPToWebServer(std::string url, std::string port)
{
    WSADATA wsaData;
    SOCKET Socket;
    SOCKADDR_IN SockAddr;
    int lineCount = 0;
    int rowCount = 0;
    struct hostent *host;
    char buffer[10000];
    int i = 0;
    int nDataLength;
    std::string website_HTML;

    // website url
    //std::string url = "http://www.e2k-attempt2.appspot.com/";
    //std::string url = "localhost:8080";

    //HTTP GET
    std::string get_http = "GET /?vmIP=vm HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log(elfSelf, "WSAStartup failed.");
        return false;
    }

    log(elfSelf, "WSAStartup OK");

    Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    host = gethostbyname(url.c_str());

    SockAddr.sin_port = htons(stoi(port));
    SockAddr.sin_family = AF_INET;

    if (host != nullptr)
    {
        SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);
        log(elfSelf, "SockAddr.sin_addr.s_addr = " + *host->h_addr);
    }
    else
    {
        inet_pton(AF_INET, "127.0.0.1", &(SockAddr.sin_addr));
        log(elfSelf, "SockAddr.sin_addr.s_addr = 127.0.0.1");
    }

    if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0) {
        log(elfSelf, "Could not connect");
        return false;
    }

    log(elfSelf, "Connect on SockAddr is OK");

    // send GET / HTTP
    send(Socket, get_http.c_str(), strlen(get_http.c_str()), 0);

    log(elfSelf, "Send is OK");

    closesocket(Socket);
    WSACleanup();

    return true;
}


int main(int argc, char *argv[])
{
    // connect to the web-server and pass self ip (implicit)
    if (argc < 3)
    {
        log(elfSelf, "enter server name, port and (probably) rooms count (default is 1)");
        return 0;
    }

    if (!implicitPassSelfIPToWebServer(argv[1], argv[2]))
    {
        log(elfSelf, "can't pass self ip to the web-server");
        //return 0;
        log(elfSelf, "but continue; client could guess my ip");
    }

    int containersCount = 1;
    if (argc > 3)
    {
         containersCount = std::stoi(argv[3], nullptr);
    }

    std::string fullPath = argv[0];
    std::wstring wFullPath;
    wFullPath.assign(fullPath.begin(), fullPath.end());

    std::vector<wchar_t> wPath(strlen(argv[0]));
    memcpy(wPath.data(), wFullPath.c_str(), strlen(argv[0]) * sizeof(wchar_t));

    //PathCchRemoveFileSpec(wPath.data(), fullPath.size());
	auto hr = PathRemoveFileSpec(wPath.data());

    // run rooms
    CRoomsContainer roomContainer(containersCount);
    if (!roomContainer.run(wPath.data(), 26001))
    {
        log(elfSelf, "can't run room communicator");
        getchar();
        return 0;
    }

    // run listener
    CListener listener(&roomContainer);
    listener.process(27000);

    getchar();

    return 0;
}

