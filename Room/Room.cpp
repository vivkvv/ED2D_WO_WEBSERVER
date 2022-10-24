// Room.cpp : Defines the entry point for the console application.
//

#include "../Common/H/common.h"
#include "ServerCommunicator.h"
#include "GameRoom.h"
#include "../Common/pugixml/pugixml.hpp"

int main(int argc, char *argv[])
{
//    getchar();

    log(elfSelf, std::string("room started; parameters =  ") + std::to_string(argc));

    std::string portString;

    if (argc >= 2)
    {
        portString = argv[1];
        log(elfSelf, std::string("port = ") + portString);
    }
    else
    {
        return 0;
    }

    std::wstring wPortString;
    wPortString.assign(portString.begin(), portString.end());

    int port = std::stoi(portString, nullptr);

    int xmlWidthInt = 800;
    int xmlHeightInt = 600;
    int xmlPlayersCountInt = 1;
    std::vector<CCharge2D> xmlCofigurationCharges2D;
    std::string roomName;
    ShowStruct forcesShowStruct;
    ShowStruct eqPotentialsShowStruct;

    if (argc == 3)
    {
        std::string xmlInput;
        xmlInput = argv[2];

        // parse xml
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(xmlInput.c_str());
        if (!result)
        {
            log(elfSelf, "can't parse xml");
            return -1;
        }

        pugi::xml_node xmlGeometry = doc.child("RoomParameters").child("Geometry");
        pugi::xml_node xmlWidth = xmlGeometry.child("Width");
        xmlWidthInt = atoi(xmlWidth.first_child().value());
        pugi::xml_node xmlHeight = xmlGeometry.child("Height");
        xmlHeightInt = atoi(xmlHeight.first_child().value());

        pugi::xml_node xmlDescription = doc.child("RoomParameters").child("Description");
        pugi::xml_node descriptionNode = xmlDescription.first_child();
        if (descriptionNode)
        {
            roomName = descriptionNode.value();
        }

        pugi::xml_node xmlStaticCharges = doc.child("RoomParameters").child("StaticCharges");

        for (pugi::xml_node staticCharge = xmlStaticCharges.first_child(); staticCharge; staticCharge = staticCharge.next_sibling())
        {
            //x, y, z, q
            auto x = atoi(staticCharge.child("x").first_child().value());
            auto y = atoi(staticCharge.child("y").first_child().value());
            auto z = atoi(staticCharge.child("z").first_child().value());
            auto q = atoi(staticCharge.child("q").first_child().value());

            xmlCofigurationCharges2D.push_back(CCharge2D(x, y, z, q, 0.0, 0.0, 0.0,
                1.0, "", OneChargeInfo_Charge2DType_ctStatic));
        }

        pugi::xml_node xmlFreeDynamicCharges = doc.child("RoomParameters").child("FreeDynamicCharges");

        for (pugi::xml_node freeDynamicCharge = xmlFreeDynamicCharges.first_child();
            freeDynamicCharge; freeDynamicCharge = freeDynamicCharge.next_sibling())
        {
            //x, y, z, q
            auto x = atoi(freeDynamicCharge.child("x").first_child().value());
            auto y = atoi(freeDynamicCharge.child("y").first_child().value());
            auto z = atoi(freeDynamicCharge.child("z").first_child().value());
            auto q = atoi(freeDynamicCharge.child("q").first_child().value());
            auto vx = atoi(freeDynamicCharge.child("vx").first_child().value());
            auto vy = atoi(freeDynamicCharge.child("vy").first_child().value());
            auto vz = atoi(freeDynamicCharge.child("vz").first_child().value());
            auto m = atoi(freeDynamicCharge.child("m").first_child().value());

            xmlCofigurationCharges2D.push_back(CCharge2D(x, y, z, q, vx, vy, vz,
                m, "", OneChargeInfo_Charge2DType_ctDynamic));
        }

        pugi::xml_node xmlPlayers = doc.child("RoomParameters").child("Players");
        pugi::xml_node xmlPlayesrCount = xmlPlayers.child("count");
        xmlPlayersCountInt = atoi(xmlPlayesrCount.first_child().value());

        pugi::xml_node xmlShow = doc.child("RoomParameters").child("Show");
        if (xmlShow)
        {
            pugi::xml_node xmlForces = xmlShow.child("Forces");
            if (xmlForces)
            {
                pugi::xml_node xmlPaletteType = xmlForces.child("PaletteType");

                forcesShowStruct.set_palettetype(strcmp(xmlPaletteType.first_child().value(), "discrete") == 0 ?
                    ShowPaletteType::sptDiscrete : ShowPaletteType::sptContinuous);

                pugi::xml_node xmlPaletteName = xmlForces.child("PaletteName");
                forcesShowStruct.set_palettename(xmlPaletteName.first_child().value());
            }

            pugi::xml_node xmlEqPotentials = xmlShow.child("EqPotentials");
            if (xmlForces)
            {
                pugi::xml_node xmlPaletteType = xmlEqPotentials.child("PaletteType");

                eqPotentialsShowStruct.set_palettetype(strcmp(xmlPaletteType.first_child().value(), "discrete") == 0 ?
                    ShowPaletteType::sptDiscrete : ShowPaletteType::sptContinuous);

                pugi::xml_node xmlPaletteName = xmlEqPotentials.child("PaletteName");
                eqPotentialsShowStruct.set_palettename(xmlPaletteName.first_child().value());
            }
        }
    }

    log(elfSelf, "creating game room on the  port " + std::to_string(port + 1000));
    CGameRoom gameRoom(roomName, xmlWidthInt, xmlHeightInt, xmlPlayersCountInt,
        forcesShowStruct, eqPotentialsShowStruct, xmlCofigurationCharges2D);
    gameRoom.start(port + 1000);

    // communicate to server
    CServerCommunicator mServerCommunicator(&gameRoom);
    mServerCommunicator.process(port);

    return 0;
}

