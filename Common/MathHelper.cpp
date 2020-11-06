#include "MathHelper.h"

#include <float.h>
#include <cmath>

using namespace DirectX;

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::PI = 3.1415926535f;

float MathHelper::AngleFromXY(const float &x, const float &y)
{
    float theta = 0.f;
    if(x > 0.f)
    {
        theta = atanf(y / x);
        if(theta < 0.0f)
        {
            theta += 2.0f * PI;
        }
    }
    else
    {
        theta = atanf(y / x) + PI;
    }

    return theta;
}


XMVECTOR MathHelper::RandUnitVec3()
{
    XMVECTOR one = XMVectorSet(1.f, 1.f, 1.f, 1.f);

    while (true)
    {
        XMVECTOR v = XMVectorSet(
            MathHelper::RandF(-1.f, 1.f),
            MathHelper::RandF(-1.f, 1.f),
            MathHelper::RandF(-1.f, 1.f),
            0);
        
        if(XMVector3Greater(XMVector3LengthSq(v), one))
        {
            continue;
        }

        return XMVector3Normalize(v);
    }
}

XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n)
{

    XMVECTOR one = XMVectorSet(1.f, 1.f, 1.f, 1.f);
    XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 0.f);

    while (true)
    {
        XMVECTOR v = XMVectorSet(
            MathHelper::RandF(-1.f, 1.f),
            MathHelper::RandF(-1.f, 1.f),
            MathHelper::RandF(-1.f, 1.f),
            0);
        
        if(XMVector3Greater(XMVector3LengthSq(v), one))
        {
            continue;
        }

        if(XMVector3Less(XMVector3LengthSq(v), zero))
        {
            continue;
        }
        
        return XMVector3Normalize(v);
    }
}