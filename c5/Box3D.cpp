#include "../Common/D3DApp.h"
#include "../Common/MathHelper.h"

using namespace DirectX;

struct ObjectConstants
{
    XMFLOAT4X4 world_view_proj = MathHelper::Identity4x4();
};

class Box3D : public D3DApp
{
    public:
        Box3D(HINSTANCE instance);
        ~Box3D();

        bool Initialize() override;

        void Update() override;

        void Draw() override;

        void OnMouseMove(WPARAM btn_state, int x, int y) override;
};


Box3D::Box3D(HINSTANCE instance): D3DApp(instance){}

Box3D::~Box3D(){}

bool Box3D::Initialize()
{
    return D3DApp::Initialize();
}

void Box3D::Update()
{

}

void Box3D::Draw()
{

}

void Box3D::OnMouseMove(WPARAM btn_state, int x, int y)
{

}