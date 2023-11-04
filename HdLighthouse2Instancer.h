#ifndef HDLIGHTHOUSE2INSTANCER_H
#define HDLIGHTHOUSE2INSTANCER_H

#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>
#include <pxr/base/gf/half.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/quatd.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/imaging/hd/vtBufferSource.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdLighthouse2Instancer : public HdInstancer
{
public:
    HdLighthouse2Instancer(HdSceneDelegate* delegate, SdfPath const& id);
    ~HdLighthouse2Instancer();

    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

    VtMatrix4dArray ComputeInstanceTransforms(SdfPath const& prototypeId);

private:
    void _SyncPrimvars(HdSceneDelegate* delegate, HdDirtyBits dirtyBits);

    TfHashMap<TfToken, HdVtBufferSource*, TfToken::HashFunctor> _primvarMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif