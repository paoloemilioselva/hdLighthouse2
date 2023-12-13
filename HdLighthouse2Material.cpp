#include "HdLighthouse2Material.h"
#include "HdLighthouse2Mesh.h"
#include "HdLighthouse2RenderDelegate.h"
#include <functional>
#include <list>
#include <regex>

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

TF_MAKE_STATIC_DATA(NdrTokenVec, _sourceTypes) {
    *_sourceTypes = {
        TfToken("OSL"),
        TfToken("lh2"),
        TfToken("mtlx")
    };
}

static float luminance(const pxr::GfVec3f& value)
{
    static constexpr pxr::GfVec3f kLuminanceFactors = pxr::GfVec3f(0.2125f, 0.7154f, 0.0721f);

    return pxr::GfDot(value, kLuminanceFactors);
}

HdLighthouse2Material::HdLighthouse2Material(SdfPath const& id, HdLighthouse2RenderDelegate* renderDelegate) :
    HdMaterial(id), _owner(renderDelegate)
{
}

HdLighthouse2Material::~HdLighthouse2Material() {}

HdDirtyBits HdLighthouse2Material::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

void HdLighthouse2Material::Sync(HdSceneDelegate* delegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
{
    const auto& id = GetId();
    //std::cout << "Syncing " << id << std::endl;

    if ((*dirtyBits & HdMaterial::DirtyResource) || (*dirtyBits & HdMaterial::DirtyParams))
    {
        //std::cout << "Updated material " << id << std::endl;
        std::lock_guard<std::mutex> guard(_owner->rendererMutex());
        auto& ltMat = _owner->GetMaterial(id, make_float3(1,1,1) );
        auto* ltScene = _owner->GetRenderer()->GetScene();

        VtValue materialValue = delegate->GetMaterialResource(id);
        if (materialValue.IsHolding<pxr::HdMaterialNetworkMap>())
        {
            auto network2Map = HdConvertToHdMaterialNetwork2(materialValue.UncheckedGet<HdMaterialNetworkMap>());
            HdMaterialToLighthouse2Material(ltScene, ltMat.material, network2Map, id);
        }
    }

    *dirtyBits &= ~HdChangeTracker::AllDirty;
}

void HdLighthouse2Material::Finalize(HdRenderParam* renderParam) {
    HdMaterial::Finalize(renderParam);
}

template <>
void HdLighthouse2Material::LtSetParam<HostMaterial::Vec3Value>(HostScene* ltScene, HostMaterial::Vec3Value& ltParam, const VtValue& vtVal)
{
    if (vtVal.IsHolding<GfVec3f>())
    {
        GfVec3f val = vtVal.UncheckedGet<GfVec3f>();
        //std::cout << " - setting color/float3 " << val << std::endl;
        ltParam = make_float3(val[0], val[1], val[2]);
    }
    else if (vtVal.IsHolding<float>())
    {
        float val = vtVal.UncheckedGet<float>();
        //std::cout << " - setting float " << val << std::endl;
        ltParam = make_float3(val, val, val);
    }
    else if (vtVal.IsHolding<SdfAssetPath>())
    {
        const SdfAssetPath& texturePath = vtVal.UncheckedGet<SdfAssetPath>();
        std::string filename = pxr::TfStringReplace(texturePath.GetResolvedPath(), "<UDIM>", "1001");
        //std::cout << " - setting " << filename << std::endl;
        //auto modFlags = HostTexture::GAMMACORRECTION | HostTexture::LINEARIZED | HostTexture::FLIPPED;
        auto modFlags = HostTexture::GAMMACORRECTION | HostTexture::FLIPPED;
        std::ifstream texFile;
        texFile.open(filename);
        if (texFile)
        {
            ltParam.textureID = ltScene->FindOrCreateTexture(filename, modFlags);
            ltScene->textures[ltParam.textureID]->ConstructMIPmaps();
        }
        else
        {
            // red, error
            ltParam = make_float3(1.0,0.0,0.0);
        }
    }
}

template<>
void HdLighthouse2Material::LtSetParam<HostMaterial::ScalarValue>(HostScene* ltScene, HostMaterial::ScalarValue& ltParam, const VtValue& vtVal)
{
    if (vtVal.IsHolding<GfVec3f>())
    {
        GfVec3f val = vtVal.UncheckedGet<GfVec3f>();
        //std::cout << " - setting color/float3 " << val << std::endl;
        ltParam = luminance(val);
    }
    else if (vtVal.IsHolding<float>())
    {
        float val = vtVal.UncheckedGet<float>();
        //std::cout << " - setting float " << val << std::endl;
        ltParam = val;
    }
    else if (vtVal.IsHolding<SdfAssetPath>())
    {
        const SdfAssetPath& texturePath = vtVal.UncheckedGet<SdfAssetPath>();
        std::string filename = pxr::TfStringReplace(texturePath.GetResolvedPath(), "<UDIM>", "1001");
        //std::cout << " - setting " << filename << std::endl;
        //auto modFlags = HostTexture::GAMMACORRECTION | HostTexture::LINEARIZED | HostTexture::FLIPPED;
        auto modFlags = HostTexture::GAMMACORRECTION | HostTexture::FLIPPED;
        std::ifstream texFile;
        texFile.open(filename);
        if (texFile)
        {
            ltParam.textureID = ltScene->FindOrCreateTexture(filename, modFlags);
            ltScene->textures[ltParam.textureID]->ConstructMIPmaps();
        }
        else
        {
            ltParam = 0.0;
        }
        //std::cout << "MIPMAP " << ltScene->textures[ltParam.textureID]->MIPlevels << std::endl;
    }
}

template<>
void HdLighthouse2Material::LtSetParam<float>(HostScene* ltScene, float& ltParam, const VtValue& vtVal)
{
    if (vtVal.IsHolding<float>())
    {
        float val = vtVal.UncheckedGet<float>();
        //std::cout << " - setting float " << val << std::endl;
        ltParam = val;
    }
}

TfTokenVector const&
HdLighthouse2Material::GetShaderSourceTypes()
{
    return *_sourceTypes;
}

void HdLighthouse2Material::HdParamToLtParam(
    lighthouse2::HostScene* i_scene,
    lighthouse2::HostMaterial* i_mat,
    const TfToken& i_parmName,
    const VtValue& i_value)
{
    if (i_parmName == TfToken("base_color") || i_parmName == TfToken("diffuseColor"))
        LtSetParam(i_scene, i_mat->color, i_value);
    else if (i_parmName == TfToken("normal"))
        LtSetParam(i_scene, i_mat->detailNormals, i_value);
    else if (i_parmName == TfToken("metalness") || i_parmName == TfToken("metallic"))
        LtSetParam(i_scene, i_mat->metallic, i_value);
    else if (i_parmName == TfToken("specular_roughness") || i_parmName == TfToken("roughness"))
        LtSetParam(i_scene, i_mat->roughness, i_value);
    else if (i_parmName == TfToken("ior"))
        LtSetParam(i_scene, i_mat->ior, i_value);
    else if (i_parmName == TfToken("clearcoat"))
        LtSetParam(i_scene, i_mat->clearcoat, i_value);
    else if (i_parmName == TfToken("clearcoatRoughness"))
    {
        auto clearcoatRoughness = lighthouse2::HostMaterial::ScalarValue();
        LtSetParam(i_scene, clearcoatRoughness, i_value);
        i_mat->clearcoatGloss.value = 1.0 - clearcoatRoughness.value;
    }
    else if (i_parmName == TfToken("transmission"))
    {
        LtSetParam(i_scene, i_mat->transmission.scale, i_value);
        i_mat->eta = 0.5;
        i_mat->ior = 1.5;
    }
    else if (i_parmName == TfToken("transmission_color"))
    {
        LtSetParam(i_scene, i_mat->transmission, i_value);
        i_mat->eta = 0.5;
        i_mat->ior = 1.5;
    }
    else if (i_parmName == TfToken("specular"))
        LtSetParam(i_scene, i_mat->specular, i_value);
    else if (i_parmName == TfToken("specular_color"))
        LtSetParam(i_scene, i_mat->specularTint, i_value);
    else if (i_parmName == TfToken("opacity"))
        LtSetParam(i_scene, i_mat->opacity, i_value);
    else if (i_parmName == TfToken("subsurface"))
        LtSetParam(i_scene, i_mat->subsurface, i_value);
    else if (i_parmName == TfToken("subsurface_scale"))
        LtSetParam(i_scene, i_mat->subsurface.scale, i_value);
    else
        std::cout << "Param " << i_parmName << " (" << TfStringify(i_value) << ") unrecognized" << std::endl;
}

void HdLighthouse2Material::FindConnectedParameters(lighthouse2::HostScene* ltScene, lighthouse2::HostMaterial* ltMat, const TfToken& conn, HdMaterialNetwork2 const& network, const SdfPath& i_name, const HdMaterialNode2& i_node)
{
    // search for parameters
    for (auto const& paramEntry : i_node.parameters)
        HdParamToLtParam(ltScene, ltMat, conn, paramEntry.second);

    // check for more parameters in connected nodes
    for (auto const& connEntry : i_node.inputConnections)
    {
        for (auto const& e : connEntry.second)
        {
            const HdMaterialNode2* upstreamNode = TfMapLookupPtr(network.nodes, e.upstreamNode);
            FindConnectedParameters(ltScene, ltMat, conn, network, e.upstreamNode, *upstreamNode);
        }
    }
}

void HdLighthouse2Material::HdMaterialToLighthouse2Material(lighthouse2::HostScene* ltScene, lighthouse2::HostMaterial* ltMat, HdMaterialNetwork2 const& network, SdfPath const& id)
{
    for (auto const& terminalEntry : network.terminals)
    {
        if (terminalEntry.first == TfToken("surface"))
        {
            const HdMaterialNode2* upstreamNode = TfMapLookupPtr(network.nodes, terminalEntry.second.upstreamNode);

            // search for parameters
            for (auto const& paramEntry : upstreamNode->parameters)
                HdParamToLtParam(ltScene, ltMat, paramEntry.first, paramEntry.second);

            // check for more parameters in connected nodes
            for (auto const& connEntry : upstreamNode->inputConnections)
            {
                for (auto const& e : connEntry.second)
                {
                    const HdMaterialNode2* otherNode = TfMapLookupPtr(network.nodes, e.upstreamNode);
                    FindConnectedParameters(ltScene, ltMat, connEntry.first, network, e.upstreamNode, *otherNode);
                }
            }
        }
    }
}