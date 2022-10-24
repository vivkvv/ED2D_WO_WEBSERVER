#pragma once

#include <string>
#include "toJS.h"

class CCharge2D
{
public:
    CCharge2D(float x, float y, float z, float q, float vx, float vy, float vz,
        float m, std::string name, OneChargeInfo_Charge2DType charge2DType);
    ~CCharge2D();

    SCharge2D msCharge2D;
    std::string mName = "";
};

