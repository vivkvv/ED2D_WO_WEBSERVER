#include "toJS.h"

#include <iostream>

#include <cmath>
#include <math.h>
#include <vector>

void checkIfNan(const float& value, const char* comment)
{
    if (isnan(value))
    {
        std::cout << comment << std::endl;
    }
};

void recalculate_test(int count,
    float* x, float* y, float* z,
    float* q,
    float* vx, float* vy, float* vz,
    float* m, char* charge2DType,
    float mQulon, bool mBCalculated, float mC, float deltaTime,
    float mWidth, float mHeight)
{
    for (int i = 0; i < count; ++i)
    {
        x[i] = 2 * x[i];
        y[i] = 2 * y[i];
        vx[i] = 2 * vx[i];
        vy[i] = 2 * vy[i];
    }
};

void recalculate(int count,
    float* x, float* y, float* z,
    float* q,
    float* vx, float* vy, float* vz,
    float* m, char* charge2DType,
    float* fex, float* fey,
    float* fbx, float* fby,
    float mQulon, bool mBCalculated,
    float mC, float deltaTime, float mWidth, float mHeight)
{
    const float c2 = mC * mC;

    std::vector<float> xNew(count);
    std::vector<float> yNew(count);
    std::vector<float> zNew(count);
    std::vector<float> vxNew(count);
    std::vector<float> vyNew(count);
    std::vector<float> vzNew(count);

    for (int i = 0; i < count; ++i)
    {
        vxNew[i] = vx[i];
        vyNew[i] = vy[i];
        xNew[i] = x[i];
        yNew[i] = y[i];
    }

    // calculate new positions and velocities for all the charges
    for (auto i = 0; i < count; ++i)
    {
        if (charge2DType[i] == OneChargeInfo_Charge2DType_ctStatic)
        {
        }
        else
        {
            float Fx = 0.0;
            float Fy = 0.0;
            float Ex = 0.0;
            float Ey = 0.0;
            float B = 0.0;

            for (auto innerIt = 0; innerIt != count; ++innerIt)
            {
                if(i == innerIt)
                {
                    continue;
                }

                float dx = x[innerIt] - x[i];
                float dy = y[innerIt] - y[i];
                float r2 = dx*dx + dy*dy;
                float r = std::sqrtf(r2);

                if (r < 10.0)
                {
                    continue;
                }

                float innerQ = q[innerIt];
                float innerRx = x[innerIt];
                float innerRy = y[innerIt];
                float innerVx = vx[innerIt];
                float innerVy = vy[innerIt];

                // E = Summs ((qi/ri)ni)
                Ex += -mQulon * innerQ * (dx / r) / r;
                checkIfNan(Ex, "Ex");
                Ey += -mQulon * innerQ * (dy / r) / r;
                checkIfNan(Ey, "Ey");

                //log(elfSelf, "sending info about starting game to Server");

                // B = Summa (qi * ( (vxi, vyi) * (ryi, - rxi)) / (mu * c * r2))
                if (mBCalculated)
                {
                    B += innerQ * (innerVx * innerRy - innerVy * innerRx) / (mC * r2);
                }
            }

            auto Fex = q[i] * Ex;
            auto Fbx = q[i] * (1 / mC) * B * (vy[i]);

            Fx = Fex + Fbx;
            checkIfNan(Fx, "Fx");

            auto Fey = q[i] * Ey;
            auto Fby = -q[i] * (1 / mC) * B * (vx[i]);
            Fy = Fey + Fby;
            checkIfNan(Fy, "Fy");

            fex[i] = Fex;
            fey[i] = Fey;
            fbx[i] = Fbx;
            fby[i] = Fby;

            float v2 = vx[i] * vx[i] + vy[i] * vy[i];
            checkIfNan(v2, "v2");

            float pxOld = m[i] * vx[i] / std::sqrt(1 - v2 / c2);
            checkIfNan(pxOld, "pxOld");
            float pyOld = m[i] * vy[i] / std::sqrt(1 - v2 / c2);
            checkIfNan(pyOld, "pyOld");

            float pxNew = pxOld + Fx * deltaTime / 2;
            checkIfNan(pxNew, "pxNew");
            float pyNew = pyOld + Fy * deltaTime / 2;
            checkIfNan(pyNew, "pyNew");

            float p2New = pxNew * pxNew + pyNew * pyNew;
            checkIfNan(p2New, "p2New");

            vxNew[i] = pxNew / std::sqrt(m[i] * m[i] + p2New / c2);
            checkIfNan(vxNew[i], "vxNew");
            vyNew[i] = pyNew / std::sqrt(m[i] * m[i] + p2New / c2);
            checkIfNan(vyNew[i], "vyNew");

            xNew[i] = x[i] + vxNew[i] * deltaTime;
            checkIfNan(xNew[i], "xNew");
            yNew[i] = y[i] + vyNew[i] * deltaTime;
            checkIfNan(yNew[i], "yNew");

            // check bounds
            if (xNew[i] < 0.0)
            {
                xNew[i] = -xNew[i];
                vxNew[i] = -vxNew[i];
            }
            else if (xNew[i] > mWidth)
            {
                xNew[i] = 2 * mWidth - xNew[i];
                vxNew[i] = -vxNew[i];
            }

            if (yNew[i] < 0.0)
            {
                yNew[i] = -yNew[i];
                vyNew[i] = -vyNew[i];
            }
            else if (yNew[i] > mHeight)
            {
                yNew[i] = 2 * mHeight - yNew[i];
                vyNew[i] = -vyNew[i];
            }
        }

        // adjusting vxNew and vyNew
        float v2New = vxNew[i] * vxNew[i] + vyNew[i] * vyNew[i];
        if (v2New >= c2)
        {
            float coeff = 0.001 + v2New / c2;
            vxNew[i] /= coeff;
            vyNew[i] /= coeff;
        }
    }

    for (int i = 0; i < count; ++i)
    {
        vx[i] = vxNew[i];
        vy[i] = vyNew[i];
        x[i] = xNew[i];
        y[i] = yNew[i];
    }

}

