#ifndef HDLIGHTHOUSE2MATERIAL_H
#define HDLIGHTHOUSE2MATERIAL_H

#include <pxr/pxr.h>
#include <pxr/imaging/hd/material.h>
#include <string>

#include "HdLighthouse2RenderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdLighthouse2Material : public HdMaterial
{
public:
    HdLighthouse2Material(SdfPath const& sprimId, HdLighthouse2RenderDelegate* renderDelegate);
    virtual ~HdLighthouse2Material() override;

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;
    virtual void Sync(HdSceneDelegate* delegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

    virtual void Finalize(HdRenderParam* renderParam) override;
    
    template <typename T>
    void LtSetParam(HostScene* ltScene, T& ltParam, const VtValue& vtVal);
    template <>
    void LtSetParam<HostMaterial::Vec3Value>(HostScene* ltScene, HostMaterial::Vec3Value& ltParam, const VtValue& vtVal);
    template <>
    void LtSetParam<HostMaterial::ScalarValue>(HostScene* ltScene, HostMaterial::ScalarValue& ltParam, const VtValue& vtVal);
    template <>
    void LtSetParam<float>(HostScene* ltScene, float& ltParam, const VtValue& vtVal);

    void HdParamToLtParam(
        lighthouse2::HostScene* i_scene,
        lighthouse2::HostMaterial* i_mat,
        const TfToken& i_parmName,
        const VtValue& i_value);

    void FindConnectedParameters(
        lighthouse2::HostScene* ltScene,
        lighthouse2::HostMaterial* ltMat,
        const TfToken& conn,
        HdMaterialNetwork2 const& network,
        const SdfPath& i_name,
        const HdMaterialNode2& i_node);

    void HdMaterialToLighthouse2Material(
        lighthouse2::HostScene* ltScene,
        lighthouse2::HostMaterial* ltMat,
        HdMaterialNetwork2 const& network,
        SdfPath const& id);


    /// Return the static list of tokens supported.
    static TfTokenVector const& GetShaderSourceTypes();

private:
    HdLighthouse2RenderDelegate* _owner;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif