#ifndef _TO_JS_H_
#define _TO_JS_H_

#include "../Common/Proto/client_listener.pb.h"

struct SCharge2D
{
    SCharge2D()
    {
    }

    SCharge2D(float x, float y, float z, float q, float vx, float vy, float vz,
        float m, OneChargeInfo_Charge2DType charge2DType)
        : mX(x), mY(y), mZ(z), mQ(q), mVx(vx), mVy(vy), mVz(vz), mM(m), mCharge2DType(charge2DType)
    {
    }

    float mX = 0.0;
    float mY = 0.0;
    float mZ = 0.0;
    float mQ = 0.0;
    float mVx = 0.0;
    float mVy = 0.0;
    float mVz = 0.0;
    float mM = 0.0;
    float mFex = 0.0;
    float mFey = 0.0;
    float mFbx = 0.0;
    float mFby = 0.0;
    OneChargeInfo_Charge2DType mCharge2DType = OneChargeInfo_Charge2DType_ctStatic;
};


struct TSCharges2DStruct
{
    int id;
    SCharge2D* charge;
};

extern "C"
{
    void recalculate_test(int count,
        float* x, float* y, float* z,
        float* q,
        float* vx, float* vy, float* vz,
        float* m, char* charge2DType,
        float mQulon, bool mBCalculated, float mC, float deltaTime,
        float mWidth, float mHeight);

    void recalculate(int count,
        float* x, float* y, float* z,
        float* q, 
        float* vx, float* vy, float* vz,
        float* m, char* charge2DType,
        float* fex, float* fey,
        float* fbx, float* fby,
        float mQulon, bool mBCalculated, float mC, float deltaTime,
        float mWidth, float mHeight);
}

#endif // _TO_JS_H_