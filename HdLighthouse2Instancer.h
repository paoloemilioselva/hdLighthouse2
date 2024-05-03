#ifndef HDLIGHTHOUSE2INSTANCER_H
#define HDLIGHTHOUSE2INSTANCER_H

#include "pxr/pxr.h"

#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/base/gf/half.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/quatd.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/gf/quaternion.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/imaging/hd/vtBufferSource.h>

class HdLighthouse2Instancer : public pxr::HdInstancer
{
public:
    HdLighthouse2Instancer(pxr::HdSceneDelegate* delegate, pxr::SdfPath const& id);
    ~HdLighthouse2Instancer();

    void Sync(pxr::HdSceneDelegate* sceneDelegate, pxr::HdRenderParam* renderParam, pxr::HdDirtyBits* dirtyBits) override;

    pxr::VtMatrix4dArray ComputeInstanceTransforms(pxr::SdfPath const& prototypeId);

private:
    void _SyncPrimvars(pxr::HdSceneDelegate* delegate, pxr::HdDirtyBits dirtyBits);

    pxr::TfHashMap<pxr::TfToken, pxr::HdVtBufferSource*, pxr::TfToken::HashFunctor> _primvarMap;
};

#endif