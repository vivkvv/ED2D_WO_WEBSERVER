#include "GameRoom.h"

#include "../Common/H/common.h"
#include "ServerCommunicator.h"
#include "../Common/Proto/client_listener.pb.h"
#include <iostream>
#include <fstream>
#include <chrono>

CGameRoom::CGameRoom(const std::string& name, int width, int height,
    int players, ShowStruct forcesShowStruct,
    ShowStruct eqPotentialsShowStruct,
    const std::vector<CCharge2D>& charges2D)
    : mPlayersCommunicator(this)
    , mWidth(width)
    , mHeight(height)
    , mForcesShowStruct(forcesShowStruct)
    , mEqPotentialsShowStruct(eqPotentialsShowStruct)
    , mXmlConfigurationCharges2D(charges2D)
{
    srand(time(NULL));

    mRoomInfo.set_roomname(name);
    mRoomInfo.set_gamestatus(RoomInfo_GameStatusEnum_gseWaiting);
    mRoomInfo.set_roomsize(players);
    mRoomInfo.set_waitingsize(players);
}

CGameRoom::~CGameRoom()
{
}

void CGameRoom::start(int port)
{
    mThread = std::make_unique<std::thread>(&CGameRoom::run, this, port);
}

int CGameRoom::getNextId()
{
    return mNextId++;
}

int CGameRoom::getNextPacketId()
{
    return mPacketId++;
}

void CGameRoom::run(int port)
{
    log(elfSelf, "start game server on the port " + std::to_string(port));
    mPlayersCommunicator.process(port);
}

void CGameRoom::sendReplayOnGameView(websocketpp::connection_hdl hdl,
    int32_t ticket)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    log(elfSelf, "sending responce on game view to Server");

    std::string host;
    uint16_t port;
    mPlayersCommunicator.getSelfAddressAndPort(host, port);

    auto conn = mServerCommunicator->get_con_from_hdl(hdl);

    RoomWrappedMessage rwm;
    ResponceOnGameView* viewResponce = new ResponceOnGameView;

    viewResponce->set_responce(mRoomInfo.gamestatus() == RoomInfo::gsePlaying);
    viewResponce->set_clientip(host);
    viewResponce->set_clientport(port);
    viewResponce->set_ticket(ticket);

    log(elfSelf, std::string("responce on view query = ") + std::to_string(viewResponce->responce()) +
        "; ticket = " + std::to_string(ticket) + "; host = " + host + "; port = " + std::to_string(port));

    rwm.set_allocated_responceview(viewResponce);

    std::ostringstream out;
    rwm.SerializeToOstream(&out);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}

void CGameRoom::sendReplyOnGameInvitation(websocketpp::connection_hdl hdl, int32_t ticket)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    log(elfSelf, "sending responce on game invitation to Server");

    std::string host;
    uint16_t port;
    mPlayersCommunicator.getSelfAddressAndPort(host, port);

    auto conn = mServerCommunicator->get_con_from_hdl(hdl);

    RoomWrappedMessage rwm;
    ResponceOnGameInvitation* invitationResponce = new ResponceOnGameInvitation;

    invitationResponce->set_responce (
        (mRoomInfo.gamestatus() == RoomInfo::gseWaiting) &&
        (mRoomInfo.waitingsize() > 0));
    invitationResponce->set_ticket(ticket);

    invitationResponce->set_clientip(host);
    invitationResponce->set_clientport(port);

    log(elfSelf, std::string("responce = ") + std::to_string(invitationResponce->responce()) +
        "; ticket = " + std::to_string(invitationResponce->ticket()) +
        "; host = " + host + "; port = " + std::to_string(port));

    rwm.set_allocated_responceinvitation(invitationResponce);

    std::ostringstream out;
    rwm.SerializeToOstream(&out);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}

void CGameRoom::onCloseHandler(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    log(elfClient, "closing handler");

    auto it = mPlayersConnections.find(hdl);

    if (it != mPlayersConnections.end())
    {
        auto data = &it->second;

        auto dataIt = mCharges2DMap.find(data->id);
        if (dataIt != mCharges2DMap.end())
        {
            mCharges2DMap.erase(dataIt);
        }

        mPlayersConnections.erase(hdl);
        mRoomInfo.set_waitingsize(mRoomInfo.roomsize() - mPlayersConnections.size());
    }

    if (mPlayersConnections.size() == 0)
    {
        mGameStop = true;
        mRoomInfo.set_gamestatus(RoomInfo_GameStatusEnum_gseWaiting);
    }

    auto itViewers = mViewersConnections.find(hdl);
    if (itViewers != mViewersConnections.end())
    {
        mViewersConnections.erase(hdl);
    }

    sendRoomStatusToServer(mServerCommunicator->toServerConnection());
}

void CGameRoom::onMessage(websocketpp::connection_hdl hdl, message_ptr msg)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    log(elfClient, "message");

    auto payload = msg->get_payload();

    ClientWrappedMessage cmw;

    if (cmw.ParseFromString(payload))
    {
        auto it = mPlayersConnections.find(hdl);
        TPlayerData* data = nullptr;
        auto chargeIt = mCharges2DMap.end();
        if (it != mPlayersConnections.end())
        {
            data = &it->second;
            chargeIt = mCharges2DMap.find(data->id);
        }

        //TPlayerData* data = &mPlayersConnections[hdl];

        if (cmw.has_ping())
        {
            log(elfClient, "ping");
        }
        else if (cmw.has_key())
        {
            KeyboardKey kk = cmw.key();
            int32_t key = kk.keyboardkey();

            if (key == 0) // pageUp
            {
                if (chargeIt == mCharges2DMap.end())
                {
                    return;
                }
                CCharge2D* charge = &chargeIt->second;
                charge->msCharge2D.mQ = std::min(charge->msCharge2D.mQ + 1.0, 50.0);
            }
            else if (key == 1) // PageDown
            {
                if (chargeIt == mCharges2DMap.end())
                {
                    return;
                }
                CCharge2D* charge = &chargeIt->second;
                charge->msCharge2D.mQ = std::max(charge->msCharge2D.mQ - 1.0, -50.0);
            }
            else if (key == 2) // arrow up
            {
                if (chargeIt == mCharges2DMap.end())
                {
                    return;
                }
                CCharge2D* charge = &chargeIt->second;
                charge->msCharge2D.mQ = std::min(charge->msCharge2D.mQ + 0.1, 50.0);
            }
            else if (key == 3) // arrow down
            {
                if (chargeIt == mCharges2DMap.end())
                {
                    return;
                }
                CCharge2D* charge = &chargeIt->second;
                charge->msCharge2D.mQ = std::max(charge->msCharge2D.mQ - 0.1, -50.0);
            }
            else if (key == 4) // get info - qulon, c, if b calculated
            {
                //sendStartConditionsToAllPlayers();
                SceneGeometry* sceneGeometry = new SceneGeometry;
                sceneGeometry->set_height(mHeight);
                sceneGeometry->set_width(mWidth);
                sceneGeometry->set_qulon(mQulon);
                sceneGeometry->set_lightvelocity(mC);
                sceneGeometry->set_ifmagnetic(mBCalculated);
                sceneGeometry->set_allocated_forcesshowstruct(new ShowStruct(mForcesShowStruct));
                sceneGeometry->set_allocated_eqpotentialsshowstruct(new ShowStruct(mEqPotentialsShowStruct));

                GameInfo* gi = new GameInfo;
                gi->set_allocated_geometry(sceneGeometry);
                if (chargeIt != mCharges2DMap.end())
                {
                    gi->set_currentchargeid(chargeIt->first);
                }
                else
                {
                    gi->set_currentchargeid(-1);
                }

                RoomWrappedToClientMessage rwtcm;
                rwtcm.set_allocated_gameinfo(gi);

                auto conn = mPlayersCommunicator.get_con_from_hdl(hdl);

                std::ostringstream out;
                rwtcm.SerializeToOstream(&out);
                conn->send(out.str(), websocketpp::frame::opcode::binary);
            }
        }
        else if (cmw.has_qulon())
        {
            if (chargeIt == mCharges2DMap.end())
            {
                return;
            }
            auto qulon = cmw.qulon();
            mQulon = qulon.qulon();
        }
        else if (cmw.has_lightvelocity())
        {
            if (chargeIt == mCharges2DMap.end())
            {
                return;
            }
            auto lightVelocity = cmw.lightvelocity();
            mC = lightVelocity.lightvelocity();
        }
        else if (cmw.has_magneticcalculated())
        {
            if (chargeIt == mCharges2DMap.end())
            {
                return;
            }
            auto magnetic = cmw.magneticcalculated();
            mBCalculated = magnetic.magneticcalculated();
        }
    }
}

bool CGameRoom::onOpenHandler(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    log(elfClient, "opening handler");

    if (mRoomInfo.gamestatus() != RoomInfo_GameStatusEnum_gseWaiting ||
        mRoomInfo.waitingsize() <= 0)
    {
        // maybe, it is viewer
        // TODO: check the ticket
        TViewerData data;
        data.enabled = false;
        mViewersConnections[hdl] = data;
        return false;
    }

    TPlayerData data;
    data.id = getNextId();

    CCharge2D charge2D(100.0, 100.0, 0.0, -50, 0.0, 0.0, 0.0,
        1.0, "", OneChargeInfo_Charge2DType_ctDynamic);

    mPlayersConnections[hdl] = data;
    mCharges2DMap.insert(std::pair<int, CCharge2D>(data.id, charge2D));

    mRoomInfo.set_waitingsize(mRoomInfo.roomsize() - mPlayersConnections.size());

    sendRoomStatusToServer(mServerCommunicator->toServerConnection());

    if (mRoomInfo.waitingsize() == 0)
    {
        new std::thread(&CGameRoom::runGame, this);
    }

    return true;
}

void CGameRoom::sendStartConditionsToAllPlayers()
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    for each (auto& connection in  mPlayersConnections)
    {
        TPlayerData data = connection.second;

        SceneGeometry* sceneGeometry = new SceneGeometry;
        sceneGeometry->set_width(mWidth);
        sceneGeometry->set_height(mHeight);
        sceneGeometry->set_qulon(mQulon);
        sceneGeometry->set_lightvelocity(mC);
        sceneGeometry->set_ifmagnetic(mBCalculated);
        sceneGeometry->set_allocated_forcesshowstruct(new ShowStruct(mForcesShowStruct));
        sceneGeometry->set_allocated_eqpotentialsshowstruct(new ShowStruct(mEqPotentialsShowStruct));

        GameInfo* gi = new GameInfo;
        gi->set_allocated_geometry(sceneGeometry);
        gi->set_currentchargeid(data.id);

        RoomWrappedToClientMessage rwtcm;
        rwtcm.set_allocated_gameinfo(gi);

        auto hdl = connection.first;

        auto conn = mPlayersCommunicator.get_con_from_hdl(hdl);

        std::ostringstream out;
        rwtcm.SerializeToOstream(&out);
        conn->send(out.str(), websocketpp::frame::opcode::binary);
    }

}

void CGameRoom::calculateNewParameters(std::chrono::milliseconds& currentTime)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    using namespace std::chrono;

    std::chrono::milliseconds oldTime = currentTime;
    currentTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    float deltaTime = (currentTime.count() - oldTime.count()) / 10.0;

    // const float kQulon = 0.5;
    // const float epsilon = 5.0;
    // const float mu = 1.0;
    // const float c = 2.0;
    //const float c2 = mC * mC;

    // http://www.physics.princeton.edu/~mcdonald/examples/2dem.pdf
    // F' = q' * (E/eps + (vy', -vx') * B / (mu *c) )
    // F = dp / dt
    // p = mv / sqrt(1 - v*v/c*c);
    // E = Summs ((qi/ri)ni)
    // B = Summa (qi * ( (vxi, vyi) * (ryi, - rxi)) / (mu * c * r2))

    std::vector<TSCharges2DStruct> chargesVector;
    for (auto it = mCharges2DMap.begin(); it != mCharges2DMap.end(); ++it)
    {
        it->first;
        CCharge2D* charge = &it->second;

        TSCharges2DStruct ch2DStruct;
        ch2DStruct.id = it->first;
        ch2DStruct.charge = &charge->msCharge2D;

        chargesVector.push_back(ch2DStruct);
    }

    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;
    std::vector<float> q;
    std::vector<float> vx;
    std::vector<float> vy;
    std::vector<float> vz;
    std::vector<float> m;
    std::vector<char> charge2DType;
    std::vector<float> fex(chargesVector.size());
    std::vector<float> fey(chargesVector.size());
    std::vector<float> fbx(chargesVector.size());
    std::vector<float> fby(chargesVector.size());

    std::vector<float> xNew(chargesVector.size());
    std::vector<float> yNew(chargesVector.size());
    std::vector<float> zNew(chargesVector.size());
    std::vector<float> vxNew(chargesVector.size());
    std::vector<float> vyNew(chargesVector.size());
    std::vector<float> vzNew(chargesVector.size());

    for (int i = 0; i < chargesVector.size(); ++i)
    {
        x.push_back(chargesVector[i].charge->mX);
        y.push_back(chargesVector[i].charge->mY);
        z.push_back(chargesVector[i].charge->mZ);
        q.push_back(chargesVector[i].charge->mQ);
        vx.push_back(chargesVector[i].charge->mVx);
        vy.push_back(chargesVector[i].charge->mVy);
        vz.push_back(chargesVector[i].charge->mVz);
        m.push_back(chargesVector[i].charge->mM);
        charge2DType.push_back(chargesVector[i].charge->mCharge2DType);
    }

    recalculate(chargesVector.size(), x.data(), y.data(), z.data(),
        q.data(), vx.data(), vy.data(), vz.data(), m.data(),
        charge2DType.data(), fex.data(), fey.data(), fbx.data(), fby.data(),
        mQulon, mBCalculated,
        mC, deltaTime, mWidth, mHeight);

    for (int i = 0; i < chargesVector.size(); ++i)
    {
        chargesVector[i].charge->mX = x[i];
        chargesVector[i].charge->mY = y[i];
        chargesVector[i].charge->mVx = vx[i];
        chargesVector[i].charge->mVy = vy[i];
        chargesVector[i].charge->mFex = fex[i];
        chargesVector[i].charge->mFey = fey[i];
        chargesVector[i].charge->mFbx = fbx[i];
        chargesVector[i].charge->mFby = fby[i];
    }

    return;
}

template <class T>
void CGameRoom::sendAllParametersToPlayerWithConnection(
    const T& connection,
    int32_t packetId)
{
    //TPlayerData data = connection.second;

    RoomToClient* rtc = new RoomToClient;
    rtc->set_packetid(packetId);

    for (auto it = mCharges2DMap.begin(); it != mCharges2DMap.end(); ++it)
    {
        CCharge2D* charge = &it->second;

        OneChargeInfo* oci = rtc->add_chargeinfo();

        oci->set_id(it->first);
        oci->set_type(charge->msCharge2D.mCharge2DType);
        oci->set_m(charge->msCharge2D.mM);
        oci->set_charge(charge->msCharge2D.mQ);
        oci->set_x(charge->msCharge2D.mX);
        oci->set_y(charge->msCharge2D.mY);
        oci->set_vx(charge->msCharge2D.mVx);
        oci->set_vy(charge->msCharge2D.mVy);
        oci->set_fex(charge->msCharge2D.mFex);
        oci->set_fey(charge->msCharge2D.mFey);
        oci->set_fbx(charge->msCharge2D.mFbx);
        oci->set_fby(charge->msCharge2D.mFby);
    }

    RoomWrappedToClientMessage rwtcm;
    rwtcm.set_allocated_roomtoclient(rtc);

    auto hdl = connection.first;

    auto conn = mPlayersCommunicator.get_con_from_hdl(hdl);

    std::ostringstream out;
    rwtcm.SerializeToOstream(&out);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}

void CGameRoom::sendAllParametersToAllPlayers()
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    int32_t currentPacketId = getNextPacketId();

    for each (auto& connection in mPlayersConnections)
    {
        sendAllParametersToPlayerWithConnection(connection, currentPacketId);
    }

    for each (auto& connection in mViewersConnections)
    {
        sendAllParametersToPlayerWithConnection(connection, currentPacketId);
    }
}

void CGameRoom::sendStartGameInfoToServer(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> mLocker(mMutex);

    log(elfSelf, "sending info about starting game to Server");

    auto conn = mServerCommunicator->get_con_from_hdl(hdl);

    RoomWrappedMessage rwm;
    StartGame* startGame = new StartGame();
    startGame->set_ticket(mServerCommunicator->getTicket());
    rwm.set_allocated_startgame(startGame);

    std::ostringstream out;
    rwm.SerializeToOstream(&out);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}

void CGameRoom::sendRoomStatusToServer(websocketpp::connection_hdl hdl)
{
//    std::unique_lock<std::mutex> mLocker(mMutex); is it possible to recursive using of mMutex?

    log(elfSelf, "sending game info to Server");

    auto conn = mServerCommunicator->get_con_from_hdl(hdl);

    RoomWrappedMessage rwm;
    RoomInfo* roomInfo = new RoomInfo(mRoomInfo);
    rwm.set_allocated_roominfo(roomInfo);

    std::ostringstream out;
    rwm.SerializeToOstream(&out);
    conn->send(out.str(), websocketpp::frame::opcode::binary);
}

void CGameRoom::setServerCommunicator(CServerCommunicator* server)
{
    mServerCommunicator = server;
}

void CGameRoom::initializeGame()
{
    mRoomInfo.set_gamestatus(RoomInfo_GameStatusEnum_gsePlaying);

    mGameStop = false;

    mPacketId = 0;

    /*
    CCharge2D testCharge(300.0, 300.0, 0.0, 50.0, 0.0, 0.0, 0.0,
        1.0, "", OneChargeInfo_Charge2DType_ctDynamic);

    mCharges2DMap.insert(std::pair<int, CCharge2D>(getNextId(), testCharge));
    */
    //CCharge2D testCharge1(500.0, 500.0, 0.0, 50.0, 0.0, 0.0, 0.0,
    //    1.0, "", OneChargeInfo_Charge2DType_ctStatic);

    for (int i = 0; i < mXmlConfigurationCharges2D.size(); ++i)
    {
        mCharges2DMap.insert(std::pair<int, CCharge2D>(getNextId(), mXmlConfigurationCharges2D[i]));
    }

    //mCharges2DMap.insert(std::pair<int, CCharge2D>(getNextId(), testCharge1));
}

void CGameRoom::closeAllConnections()
{
    std::unique_lock<std::mutex> mLocker(mMutex);
    for (auto it = mPlayersConnections.begin(); it != mPlayersConnections.end(); ++it)
    {
        auto conn = mPlayersCommunicator.get_con_from_hdl(it->first);
        conn->close(0, "game end");
    }
}

void CGameRoom::runGame()
{
    initializeGame();

    sendRoomStatusToServer(mServerCommunicator->toServerConnection());
    sendStartGameInfoToServer(mServerCommunicator->toServerConnection());

    // send positions, velocities and charges to all the players
    using namespace std::chrono;
    milliseconds currentTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    sendStartConditionsToAllPlayers();
    sendAllParametersToAllPlayers();

    for (int i = 0; i < 1000000; ++i)
    {
        calculateNewParameters(currentTime);
        sendAllParametersToAllPlayers();
        log(elfSelf, "game step");
        Sleep(20);
        if (mGameStop)
        {
            break;
        }
    }
    log(elfSelf, "game stop");
    mCharges2DMap.clear();

    // send results ???

    closeAllConnections();
}

void CGameRoom::stopGame()
{
    log(elfSelf, "game end");
}
