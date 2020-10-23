#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ostream& XM_CALLCONV operator<<(ostream& os, FXMVECTOR v)
{
    XMFLOAT3 dest;
    XMStoreFloat3(&dest, v);
    os << "(" << dest.x << "," << dest.y << "," << dest.z << ")";
    return os;
}


int main()
{
    cout.precision(8);
    // check SSE2 support
    if(!XMVerifyCPUSupport())
    {
        cout << "directx math not supported!" << endl;
        return 0;
    }

    XMVECTOR u = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    XMVECTOR n = XMVector3Normalize(u);

    float LU = XMVectorGetX(XMVector3Length(n));

    cout << LU << endl;

    if(LU == 1.0f)
        cout << "Length 1" << endl;
    else
        cout << "Length not 1" << endl;

    float powLU = powf(LU, 1.0e6f);
    cout << "LU^(10^6) = " << powLU << endl;

    // exercises
    //1. 
    cout << "Exercise 1" << endl;
    XMVECTOR x = XMVectorSet(1.f, 2.f, 0.f, 0.f);
    XMVECTOR y = XMVectorSet(3.f, -4.f, 0.f, 0.f);

    cout << "x - y = " << (x - y) << endl;
    cout << "x + y = " << x + y << endl;
    cout << "2x + 0.5v = " << (2 * x + 0.5 * y) << endl;

    //2.
    cout << "Exercise 2" << endl;
    x = XMVectorSet(-1.f, 3.f, 2.f, 0.f);
    y = XMVectorSet(3.f, -4.f, 1.f, 0.f);

    cout << "x + y = " << (x + y) << endl;
    cout << "x - y = " << (x - y) << endl;
    cout << "3x + 2y = " << (3 * x + 2 * y) << endl;
    cout << "-2x + y = " << (-2 * x + y) << endl;

}