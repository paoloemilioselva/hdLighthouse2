#ifndef HDLIGHTHOUSE2DOMELIGHT_H
#define HDLIGHTHOUSE2DOMELIGHT_H

#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/pxr.h>
#include <string>

#include "HdLighthouse2RenderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdLighthouse2DomeLight : public HdLight
{
public:
    HdLighthouse2DomeLight(SdfPath const& sprimId, HdLighthouse2RenderDelegate* renderDelegate);
    ~HdLighthouse2DomeLight() override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;
    void Sync(HdSceneDelegate* delegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

private:
    HdLighthouse2RenderDelegate* _owner;
    std::string _environmentImageFilePath;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif