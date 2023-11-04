#ifndef HDLIGHTHOUSE2AREALIGHT_H
#define HDLIGHTHOUSE2AREALIGHT_H

#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>
#include <string>

#include "HdLighthouse2RenderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdLighthouse2AreaLight : public HdLight
{
public:
    HdLighthouse2AreaLight(SdfPath const& sprimId, HdLighthouse2RenderDelegate* renderDelegate);
    ~HdLighthouse2AreaLight() override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;
    void Sync(HdSceneDelegate* delegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

private:
    HdLighthouse2RenderDelegate* _owner;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif