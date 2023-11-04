#include "HdLighthouse2DomeLight.h"


HdLighthouse2DomeLight::HdLighthouse2DomeLight(
    SdfPath const& rprimId, HdLighthouse2RenderDelegate* renderDelegate) :
    HdLight(rprimId), _owner(renderDelegate)
{
}

HdLighthouse2DomeLight::~HdLighthouse2DomeLight()
{
}

HdDirtyBits HdLighthouse2DomeLight::GetInitialDirtyBitsMask() const
{
    return HdLight::AllDirty;
}

void HdLighthouse2DomeLight::Sync(
    HdSceneDelegate* delegate, HdRenderParam* /* renderParam */, HdDirtyBits* dirtyBits)
{
    const auto& id = GetId();

    VtValue envFilePathVal = delegate->GetLightParamValue(id, pxr::HdLightTokens->textureFile);
    auto envFilePath = envFilePathVal.Get<pxr::SdfAssetPath>().GetResolvedPath();

    if (_environmentImageFilePath != envFilePath)
    {
        std::lock_guard<std::mutex> guard(_owner->rendererMutex());

        _environmentImageFilePath = envFilePath;
        if (!_environmentImageFilePath.empty())
        {
            std::cout << "Creating dome " << _environmentImageFilePath << std::endl;
            if (_owner->GetRenderer()->GetScene()->sky == nullptr)
                _owner->GetRenderer()->GetScene()->sky = new HostSkyDome();
            _owner->GetRenderer()->GetScene()->sky->Load(_environmentImageFilePath.c_str());
            _owner->GetRenderer()->GetScene()->sky->MarkAsDirty();
        }
        else
        {
            delete _owner->GetRenderer()->GetScene()->sky;
        }
    }

    if (*dirtyBits & (DirtyTransform))
    {
        std::lock_guard<std::mutex> guard(_owner->rendererMutex());

        GfMatrix4d transposedIblXform = delegate->GetTransform(id).GetTranspose();
        for (int i = 0; i < 16; ++i)
        {
            _owner->GetRenderer()->GetScene()->sky->worldToLight.cell[i] = transposedIblXform.data()[i];
        }
        _owner->GetRenderer()->GetScene()->sky->worldToLight = mat4::RotateX(-PI / 2.0) * _owner->GetRenderer()->GetScene()->sky->worldToLight * mat4::RotateY((PI / 2.0) + (PI / 7.0));
    }

    if (dirtyBits) {
        *dirtyBits = HdChangeTracker::Clean;
    }
}