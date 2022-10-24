#include "Listener.h"

#include "../Common/H/common.h"
#include "RoomsContainer.h"


CListener::CListener(CRoomsContainer* roomContainer)
    : CWebSocketServer()
    , mRoomContainer(roomContainer)
{
    mRoomContainer->setClientsListener(this);
}

CListener::~CListener()
{
}

int CListener::getIntFromHandle(websocketpp::connection_hdl hdl)
{
    auto* ptr = hdl._Get();
    auto* ptr_int = static_cast<int*>(ptr);
    return *ptr_int;
}

int32_t CListener::generateNewTicket()
{
    return mCurrentTicket++;
}

void CListener::responceOnViewGame(ResponceOnGameView& responce)
{
    auto it = mViewersConnections.find(responce.ticket());
    if (it == mViewersConnections.end())
    {
        return;
    }

    websocketpp::connection_hdl hdl = it->second;

    ListenerToClientWrappedMessage lcwm;

    InvitationToViewGame* itg = new InvitationToViewGame();
    itg->set_clientip(responce.clientip());
    itg->set_clientport(responce.clientport());
    itg->set_ticket(responce.ticket());

    lcwm.set_allocated_invitationtoview(itg);

    std::ostringstream out;
    lcwm.SerializeToOstream(&out);

    auto conn = get_con_from_hdl(hdl);
    conn->send(out.str(), websocketpp::frame::opcode::binary);

    mViewersConnections.erase(it);
}

void CListener::responceOnInvitationToGame(ResponceOnGameInvitation& responce)
{
    auto it = mPlayConnections.find(responce.ticket());
    if (it == mPlayConnections.end())
    {
        return;
    }

    websocketpp::connection_hdl hdl = it->second;

    ListenerToClientWrappedMessage lcwm;

    InvitationToPlayGame* itg = new InvitationToPlayGame();
    itg->set_clientip(responce.clientip());
    itg->set_clientport(responce.clientport());
    itg->set_ticket(responce.ticket());

    lcwm.set_allocated_invitationtoplay(itg);

    std::ostringstream out;
    lcwm.SerializeToOstream(&out);

    auto conn = get_con_from_hdl(hdl);
    conn->send(out.str(), websocketpp::frame::opcode::binary);

    mPlayConnections.erase(it);
}

void CListener::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    auto conn = get_con_from_hdl(hdl);
    auto port = conn->get_port();
    auto host = conn->get_host();

    auto rep = conn->get_remote_endpoint();

    boost::system::error_code ec;
    auto localEndpoint = conn->get_raw_socket().local_endpoint();
    auto remoteEndpoint = conn->get_remote_endpoint();

    log(elfClient, std::string("error code = ") + ec.message() + std::string("; local port = ") +
        std::to_string(localEndpoint.port()) + std::string("; local address ") +
        localEndpoint.address().to_string() + std::string("; remote = ") + remoteEndpoint);

    auto payload = msg->get_payload();

    ClientQueryRequest cqr;

    if (cqr.ParseFromString(payload))
    {
        if (cqr.has_playrequest())
        {
            auto ticket = generateNewTicket();
            {
                mPlayConnections.insert(std::pair<int, websocketpp::connection_hdl>(ticket, hdl));
            }
            auto& pqr = cqr.playrequest();
            auto roomIp = pqr.ip();
            auto roomPort = pqr.port();
            mRoomContainer->passPlayersToRoom(port, host, ticket, roomIp, roomPort);
        }
        else if (cqr.has_viewrequest())
        {
            auto ticket = generateNewTicket();
            {
                mViewersConnections.insert(std::pair<int, websocketpp::connection_hdl>(ticket, hdl));
            }
            auto& vr = cqr.viewrequest();
            auto roomIp = vr.ip();
            auto roomPort = vr.port();
            mRoomContainer->passViewerToRoom(port, host, ticket, roomIp, roomPort);
        }
        else if (cqr.has_rtcadminrequest()) // request from user as admin on new webRTC game
        {
            auto ticket = generateNewTicket();
            auto rtcGameAdminRequest = cqr.rtcadminrequest();
            auto gameName = rtcGameAdminRequest.gamename();
            auto roomSize = rtcGameAdminRequest.roomsize();
            WebRTCGames game;
            game.mGameName = gameName;
            game.roomSize = roomSize;
            WebRTCConnectionInfo rtcInfo;
            rtcInfo.intConnectionHandle = getIntFromHandle(hdl);
            rtcInfo.connection_hdl = hdl;
            rtcInfo.playerStatus = WebRTCPlayerStatus::rtcPlayerAdmin;
            game.mRTCConnections.push_back(rtcInfo);
            webRTCGames.insert(std::pair<int, WebRTCGames>(ticket, game));
            sendAllInfoAboutTheRTCGame(ticket);
        }
        else if (cqr.has_rtcclientrequest()) // request from user as client on new webRTC game
        {
            auto rtcGameClientRequest = cqr.rtcclientrequest();
            auto ticket = rtcGameClientRequest.id();
            auto itGame = webRTCGames.find(ticket);
            if (itGame == webRTCGames.end())
            {
                sendRefuseAboutRTCGame(hdl, ticket, WebRTCGameRefuseReasons::rtcRefuseNoGameWithId);
            }
            else
            {
                if (itGame->second.mRTCConnections.size() < itGame->second.roomSize)
                {
                    // check if this player is one of the players !!!
                    WebRTCConnectionInfo rtcInfo;
                    rtcInfo.intConnectionHandle = getIntFromHandle(hdl);
                    rtcInfo.connection_hdl = hdl;
                    rtcInfo.playerStatus = WebRTCPlayerStatus::rtcPlayerClient;
                    itGame->second.mRTCConnections.push_back(rtcInfo);
                    sendAllInfoAboutTheRTCGame(ticket);
                }
                else
                {
                    sendRefuseAboutRTCGame(hdl, ticket, WebRTCGameRefuseReasons::rtcRefuseRoomIsFull);
                }
            }
        }
        else if (cqr.has_rtcoffer())
        {
            // 1. take offer
            auto gameID = cqr.rtcoffer().gamertcid();
            auto offerString = cqr.rtcoffer().offer();
            auto itGame = webRTCGames.find(gameID);
            // 2. check, if sender is admin
            auto handle = getIntFromHandle(hdl);
            if (itGame != webRTCGames.end())
            {
                const WebRTCGames& game = itGame->second;
                bool senderIsAdmin = false;
                for (int i = 0; i < game.mRTCConnections.size(); ++i)
                {
                    if (game.mRTCConnections[i].intConnectionHandle == handle &&
                        game.mRTCConnections[i].playerStatus == WebRTCPlayerStatus::rtcPlayerAdmin)
                    {
                        senderIsAdmin = true;
                        break;
                    }
                }
                // 3. send offer to all the clients
                if (senderIsAdmin)
                {
                    for (int i = 0; i < game.mRTCConnections.size(); ++i)
                    {
                        if (game.mRTCConnections[i].intConnectionHandle != handle &&
                            game.mRTCConnections[i].playerStatus == WebRTCPlayerStatus::rtcPlayerClient)
                        {
                            sendOfferToWebRTCClient(game.mRTCConnections[i].connection_hdl, gameID, offerString);
                        }
                    }
                }
            }
        }
        else if (cqr.has_rtcanswer())
        {
            // 1. take answer
            auto gameID = cqr.rtcanswer().gamertcid();
            auto answerString = cqr.rtcanswer().answer();
            auto itGame = webRTCGames.find(gameID);
            // 2. check, if sender is client
            auto handle = getIntFromHandle(hdl);
            if (itGame != webRTCGames.end())
            {
                const WebRTCGames& game = itGame->second;
                bool senderIsClient = false;
                for (int i = 0; i < game.mRTCConnections.size(); ++i)
                {
                    if (game.mRTCConnections[i].intConnectionHandle == handle &&
                        game.mRTCConnections[i].playerStatus == WebRTCPlayerStatus::rtcPlayerClient)
                    {
                        senderIsClient = true;
                        break;
                    }
                }
                // 3. send offer to all the clients
                if (senderIsClient)
                {
                    for (int i = 0; i < game.mRTCConnections.size(); ++i)
                    {
                        if (game.mRTCConnections[i].intConnectionHandle != handle &&
                            game.mRTCConnections[i].playerStatus == WebRTCPlayerStatus::rtcPlayerAdmin)
                        {
                            sendAnswerToWebRTCAdmin(game.mRTCConnections[i].connection_hdl, gameID, answerString);
                            break;
                        }
                    }
                }
            }
        }
    }
    else
    {
        log(elfClient, "close connection with client ; reason: unknown message " + msg->get_payload());
    }
}

void CListener::on_close_handler(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);
    auto conn = get_con_from_hdl(hdl);
    auto port = conn->get_port();
    auto host = conn->get_host();

    log(elfClient, "connect with client ip = " + host + " and port = " + std::to_string(port) + " is closed");

    int handle = getIntFromHandle(hdl);
    auto it = mSubscribersConnections.find(handle);
    if (it != mSubscribersConnections.end())
    {
        // update or delete webrtc games
        for (auto itRTCGame = webRTCGames.begin(); itRTCGame != webRTCGames.end();)
        {
            WebRTCGames& game = itRTCGame->second;
            auto handle = getIntFromHandle(hdl);
            bool flgFound = false;
            for (int i = 0; i < game.mRTCConnections.size(); ++i)
            {
                if (game.mRTCConnections[i].intConnectionHandle == handle)
                {
                    int gameID = itRTCGame->first;
                    if (game.mRTCConnections[i].playerStatus == WebRTCPlayerStatus::rtcPlayerAdmin)
                    {
                        flgFound = true;
                        //game.mRTCConnections.erase(game.mRTCConnections.begin() + i);
                        itRTCGame = webRTCGames.erase(itRTCGame);
                        sendAllInfoAboutTheRTCGame(gameID);
                    }
                    else if (game.mRTCConnections[i].playerStatus == WebRTCPlayerStatus::rtcPlayerClient)
                    {
                        flgFound = true;
                        game.mRTCConnections.erase(game.mRTCConnections.begin() + i);
                        sendAllInfoAboutTheRTCGame(gameID);
                    }
                    break;
                }
            }

            if (!flgFound)
            {
                ++itRTCGame;
            }

        }

        mSubscribersConnections.erase(it);
    }
}

void CListener::on_open_handler(websocketpp::connection_hdl hdl)
{
    auto conn = get_con_from_hdl(hdl);
    auto port = conn->get_port();
    auto host = conn->get_host();

    log(elfClient, "connect with client ip = " + host + " and port = " + std::to_string(port));
    mSubscribersConnections.insert(std::pair<int, websocketpp::connection_hdl>(getIntFromHandle(hdl), hdl));

    // send all info about all the current games to only this client
    sendAllInfoAboutAllTheGames(hdl);
    sendAllInfoAboutTheRTCGames(hdl);
}

void CListener::sendAllInfoAboutTheGame(websocketpp::connection_hdl hdl, unsigned short port)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    RoomInfo roomInfo;
    if (mRoomContainer->getAllInfoAboutTheGame(port, roomInfo))
    {
        ListenerToClientWrappedMessage lcwm;

        ListenerToClientGamesStates* lcgs = new ListenerToClientGamesStates;

        GameState* gs = lcgs->add_gamestate();
        gs->set_roomport(port);
        RoomInfo* ri = new RoomInfo(roomInfo);
        gs->set_allocated_roominfo(ri);

        lcwm.set_allocated_states(lcgs);

        std::ostringstream out;
        lcwm.SerializeToOstream(&out);

        // send this info to all the clients
        for (auto const& it: mSubscribersConnections)
        {
            auto conn = get_con_from_hdl(it.second);
            conn->send(out.str(), websocketpp::frame::opcode::binary);
        }
    }

}

void CListener::sendAllInfoAboutTheRTCGames(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    for each (auto& rtcGame in webRTCGames)
    {
        int gameID = rtcGame.first;
        sendAllInfoAboutTheRTCGame(gameID);
    }

}

void CListener::sendAnswerToWebRTCAdmin(websocketpp::connection_hdl hdl, int32_t ticket,
    std::string answerString)
{
    ListenerToClientWrappedMessage lcwm;

    RTCClient2AdminAnswer* c2a = new RTCClient2AdminAnswer;
    c2a->set_gamertcid(ticket);
    c2a->set_answer(answerString);

    lcwm.set_allocated_rtcanswer(c2a);

    std::ostringstream out;
    lcwm.SerializeToOstream(&out);

    auto conn = get_con_from_hdl(hdl);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}


void CListener::sendOfferToWebRTCClient(websocketpp::connection_hdl hdl, int32_t ticket,
    std::string offerString)
{
    ListenerToClientWrappedMessage lcwm;

    RTCAdmin2ClientOffer* a2c = new RTCAdmin2ClientOffer;
    a2c->set_gamertcid(ticket);
    a2c->set_offer(offerString);

    lcwm.set_allocated_rtcoffer(a2c);

    std::ostringstream out;
    lcwm.SerializeToOstream(&out);

    auto conn = get_con_from_hdl(hdl);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}

void CListener::sendRefuseAboutRTCGame(websocketpp::connection_hdl hdl, int32_t ticket,
    WebRTCGameRefuseReasons reason)
{
    ListenerToClientWrappedMessage lcwm;

    ListenerToClientsWebRTCRefuse* lcWebRTCRefuse = new ListenerToClientsWebRTCRefuse;

    lcWebRTCRefuse->set_gamertcid(ticket);
    lcWebRTCRefuse->set_refusereason(reason);

    lcwm.set_allocated_rtcrefuse(lcWebRTCRefuse);

    std::ostringstream out;
    lcwm.SerializeToOstream(&out);

    auto conn = get_con_from_hdl(hdl);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}


void CListener::sendAllInfoAboutTheRTCGame(int32_t ticket)
{
//    std::unique_lock<std::mutex> mLocker(mMutex);
    std::string gameName = "";
    int32_t roomSize = 0;
    int32_t currentPlayersCount = -1;
    WebRTCPlayerStatus currentPlayerStatus = WebRTCPlayerStatus::rtcPlayerNone;

    auto itRTCGame = webRTCGames.find(ticket);
    if (itRTCGame != webRTCGames.end())
    {
        gameName = itRTCGame->second.mGameName;
        roomSize = itRTCGame->second.roomSize;
        currentPlayersCount = itRTCGame->second.mRTCConnections.size();
    }

    // send this info to all the clients
    for (auto const& it : mSubscribersConnections)
    {
        ListenerToClientWrappedMessage lcwm;

        ListenerToClientsWebRTCGamesStates* lcWebRTC = new ListenerToClientsWebRTCGamesStates;

        auto rtcState = lcWebRTC->add_rtcstate();
        rtcState->set_gamertcid(ticket);
        rtcState->set_gamename(gameName);
        rtcState->set_roomsize(roomSize);
        rtcState->set_currentplayerscount(currentPlayersCount);
        rtcState->set_playerstatus(currentPlayerStatus);

        auto conn = get_con_from_hdl(it.second);
        auto subscriberHandle = getIntFromHandle(conn);

        for (int i = 0; i < currentPlayersCount; ++i)
        {
            auto& rtcConnection = itRTCGame->second.mRTCConnections[i];

            //if (!rtcConnection.playerHdl.owner_before(conn) &&
            //    !it.second.owner_before(rtcConnection.playerHdl))
            if (subscriberHandle == rtcConnection.intConnectionHandle)
            {
                rtcState->set_playerstatus(rtcConnection.playerStatus);
                break;
            }
        }

        lcwm.set_allocated_rtcstates(lcWebRTC);
        std::ostringstream out;
        lcwm.SerializeToOstream(&out);


        conn->send(out.str(), websocketpp::frame::opcode::binary);
    }
}

void CListener::sendAllInfoAboutAllTheGames(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    std::vector<GameState> gamesState;
    mRoomContainer->getAllInfoAboutAllTheGames(gamesState);

    ListenerToClientWrappedMessage lcwm;

    ListenerToClientGamesStates* lcgs = new ListenerToClientGamesStates;

    for (int i = 0; i < gamesState.size(); ++i)
    {
        GameState* gs = lcgs->add_gamestate();
        gs->set_roomport(gamesState[i].roomport());
        RoomInfo* ri = new RoomInfo(gamesState[i].roominfo());
        gs->set_allocated_roominfo(ri);
    }

    lcwm.set_allocated_states(lcgs);

    std::ostringstream out;
    lcwm.SerializeToOstream(&out);

    auto conn = get_con_from_hdl(hdl);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}