#ifndef HDLIGHTHOUSE2CAMERA_H
#define HDLIGHTHOUSE2CAMERA_H

#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>
#include <string>

#include "HdLighthouse2RenderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdLighthouse2Camera : public HdCamera
{
public:
    HdLighthouse2Camera(SdfPath const& sprimId, HdLighthouse2RenderDelegate* renderDelegate);
    ~HdLighthouse2Camera() override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;
    void Sync(HdSceneDelegate* delegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

    HdTimeSampleArray<GfMatrix4d, 16> const&
        GetTimeSampleXforms() const {
        return _sampleXforms;
    }

private:
    HdLighthouse2RenderDelegate* _owner;
    HdTimeSampleArray<GfMatrix4d, 16> _sampleXforms;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif