#include "Charge2D.h"

CCharge2D::CCharge2D(float x, float y, float z, float q, float vx, float vy,
    float vz, float m, std::string name, OneChargeInfo_Charge2DType charge2DType)
    : msCharge2D(x, y, z, q, vx, vy, vz, m, charge2DType), mName(name)
{
}

CCharge2D::~CCharge2D()
{
}


