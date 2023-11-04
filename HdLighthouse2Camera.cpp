#include "HdLighthouse2Camera.h"

#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/matrix3d.h>


HdLighthouse2Camera::HdLighthouse2Camera(
    SdfPath const& rprimId, HdLighthouse2RenderDelegate* renderDelegate) :
    HdCamera(rprimId), _owner(renderDelegate)
{
}

HdLighthouse2Camera::~HdLighthouse2Camera()
{
}

HdDirtyBits HdLighthouse2Camera::GetInitialDirtyBitsMask() const
{
    return HdCamera::DirtyParams;
}

void HdLighthouse2Camera::Sync(
    HdSceneDelegate* delegate, HdRenderParam* renderParam , HdDirtyBits* dirtyBits)
{
    // this is triggered for all cameras in the scene AND for the "viewport" camera
    // so, unless we look for a specific camera and only update that, we
    // should do the official update for the "viewport" camera directly in RenderPass
    // instead of here.
    // This is here for future usage/reference

    const auto& id = GetId();
    //std::cout << "Syncing " << id << std::endl;

    const HdDirtyBits bits = *dirtyBits;
    if (bits & DirtyTransform) 
    {
        delegate->SampleTransform(id, &_sampleXforms);
    }

    // Sync baseclass
    HdCamera::Sync(delegate, renderParam, dirtyBits);

    // We don't need to clear the dirty bits since HdCamera::Sync always clears
    // all the dirty bits.
}