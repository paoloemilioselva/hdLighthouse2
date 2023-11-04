#include "HdLighthouse2AreaLight.h"

#include <pxr/base/gf/matrix4f.h>


HdLighthouse2AreaLight::HdLighthouse2AreaLight(
    SdfPath const& rprimId, HdLighthouse2RenderDelegate* renderDelegate) :
    HdLight(rprimId), _owner(renderDelegate)
{
}

HdLighthouse2AreaLight::~HdLighthouse2AreaLight()
{
}

HdDirtyBits HdLighthouse2AreaLight::GetInitialDirtyBitsMask() const
{
    return HdLight::AllDirty;
}

void HdLighthouse2AreaLight::Sync(
    HdSceneDelegate* delegate, HdRenderParam* /* renderParam */, HdDirtyBits* dirtyBits)
{
    // if anything is dirty, we update the whole light
    // it should be fast enough.
    // As long as we reset dirtybits at the end

    const auto& id = GetId();

    std::lock_guard<std::mutex> guard(_owner->rendererMutex());

    // this will always update the material attached to the light
    // to change its intensity/exposure/color
    //
    GfVec3f lightColor = delegate->GetLightParamValue(id, HdLightTokens->color).Get<pxr::GfVec3f>(); 
    float intensity = delegate->GetLightParamValue(id, HdLightTokens->intensity).Get<float>();
    if (intensity == 0.0)
        intensity = 1.0;
    float exposure = delegate->GetLightParamValue(id, HdLightTokens->exposure).Get<float>();
    float width = delegate->GetLightParamValue(id, HdLightTokens->width).Get<float>();
    float height = delegate->GetLightParamValue(id, HdLightTokens->height).Get<float>();
    intensity *= powf(2.0f, exposure);
    VtValue normalizeVal = delegate->GetLightParamValue(id, HdLightTokens->normalize);
    VtValue angleDegVal = delegate->GetLightParamValue(id, HdLightTokens->angle);
    if (!normalizeVal.GetWithDefault<bool>(false))
    {
        float area = 1.0f;
        if (angleDegVal.IsHolding<float>())
        {
            // Calculate solid angle area to normalize intensity.
            float angleRadians = angleDegVal.Get<float>() / 180.0f * static_cast<float>(M_PI);
            float solidAngleSteradians =
                2.0f * static_cast<float>(M_PI) * (1.0f - cos(angleRadians / 2.0f));
            area = solidAngleSteradians;
        }
        intensity *= area;
    }
    auto lightFinalColor = make_float3(lightColor[0] * intensity, lightColor[1] * intensity, lightColor[2] * intensity);

    // material and light-mesh pointing at the same id/assetpath
    auto& mesh = _owner->GetLight(id);
    auto& ltMat = _owner->GetMaterial(id, lightFinalColor);
    _owner->BindMeshToMaterial(id, id);

    // apply new color
    //
    ltMat.material->color = lightFinalColor;
    ltMat.material->metallic = HostMaterial::ScalarValue(0.0f);
    ltMat.material->roughness = HostMaterial::ScalarValue(0.0f);
    ltMat.material->specular = HostMaterial::ScalarValue(0.0f);

    // create quad if needed
    //
    if (mesh.indices.size() == 0 || mesh.vertices.size() == 0)
    {
        // Make sure we have all triangles
        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(1);
        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(3);
        mesh.indices.emplace_back(4);
        mesh.indices.emplace_back(5);

        mesh.vertices.emplace_back(make_float3(-width / 2.0, -height / 2.0, 0));
        mesh.vertices.emplace_back(make_float3(-width / 2.0, height / 2.0, 0));
        mesh.vertices.emplace_back(make_float3(width / 2.0, -height / 2.0, 0));

        mesh.vertices.emplace_back(make_float3(width / 2.0, -height / 2.0, 0));
        mesh.vertices.emplace_back(make_float3(-width / 2.0, height / 2.0, 0));
        mesh.vertices.emplace_back(make_float3(width / 2.0, height / 2.0, 0));

        const auto& n = make_float3(0, -1, 0);
        mesh.normals.emplace_back(n);
        mesh.normals.emplace_back(n);
        mesh.normals.emplace_back(n);
        mesh.normals.emplace_back(n);
        mesh.normals.emplace_back(n);
        mesh.normals.emplace_back(n);

        const auto& uv = make_float2(0, 0);
        mesh.uvs.emplace_back(uv);
        mesh.uvs.emplace_back(uv);
        mesh.uvs.emplace_back(uv);
        mesh.uvs.emplace_back(uv);
        mesh.uvs.emplace_back(uv);
        mesh.uvs.emplace_back(uv);
    }
    else
    {
        if (*dirtyBits & (DirtyParams))
        {
            mesh.dirtyMesh = true;
            mesh.vertices[0] = make_float3(-width / 2.0, -height / 2.0, 0);
            mesh.vertices[1] = (make_float3(-width / 2.0, height / 2.0, 0));
            mesh.vertices[2] = (make_float3(width / 2.0, -height / 2.0, 0));
            mesh.vertices[3] = (make_float3(width / 2.0, -height / 2.0, 0));
            mesh.vertices[4] = (make_float3(-width / 2.0, height / 2.0, 0));
            mesh.vertices[5] = (make_float3(width / 2.0, height / 2.0, 0));
        }
    }

    // apply transform
    //
    if (*dirtyBits & (DirtyTransform))
    {
        mesh.dirtyTransform = true;
        const auto& transposed = GfMatrix4f(delegate->GetTransform(id)).GetTranspose();
        if (mesh.transforms.size() == 0)
            mesh.transforms.resize(1);
        for (int i = 0; i < 16; ++i)
        {
            mesh.transforms[0].cell[i] = transposed.data()[i];
        }
    }
    
    if (dirtyBits) {
        *dirtyBits = HdChangeTracker::Clean;
    }
}