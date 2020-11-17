#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <cstdint>

class MathHelper
{
public:
    static float RandF()
    {
        return (float)(rand()) / (float)RAND_MAX;
    }

    static float RandF(const float& a, const float& b)
    {
        return a + RandF() * (a - b);
    }

    static int Rand(const int& a, const int& b)
    {
        return a + rand() % ((b - a) + 1);
    }

    template<typename T>
    static T Min(const T& a, const T& b)
    {
        return a < b? a : b;
    }
    
    template<typename T>
    static T Max(const T& a, const T& b)
    {
        return a > b? a : b;
    }
    
    template<typename T>
    static T Lerp(const T& a, const T& b, const float& t)
    {
        return a + (b - a) * t;
    }

    template<typename T>
    static T Clamp(const T& x, const T& low, const T& high)
    {
        return x < low ? low : (x > high ? high : x);
    }

    //球坐标转换到笛卡尔坐标
    static DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi)
    {
        return DirectX::XMVectorSet(
            radius * sinf(phi) * cosf(theta),
            radius * cosf(phi),
            radius * sinf(phi) * sinf(theta),
            1.0f
        );
    }

    static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX m)
    {
        DirectX::XMMATRIX a = m;
        a.r[3] = DirectX::XMVectorSet(0.f, 0.f, 0.f, 1.f);

        DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(a);
        return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, a));
    }

    static DirectX::XMFLOAT4X4 Identity4x4()
    {
        static DirectX::XMFLOAT4X4 i(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );

        return i;
    }

    static float AngleFromXY(const float& x, const float& y);
    static DirectX::XMVECTOR RandUnitVec3();
    static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);

    static const float Infinity;
    static const float PI;
};
