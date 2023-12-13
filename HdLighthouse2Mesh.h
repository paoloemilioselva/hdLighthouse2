#ifndef HDLIGHTHOUSE2MESH_H
#define HDLIGHTHOUSE2MESH_H

#include <pxr/pxr.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/vertexAdjacency.h>
#include <pxr/imaging/hd/vtBufferSource.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/imaging/hd/meshUtil.h>

#include "HdLighthouse2RenderDelegate.h"
#include "HdLighthouse2Material.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdLighthouse2Mesh final : public HdMesh 
{
public:
    HF_MALLOC_TAG_NEW("new HdLighthouse2Mesh");

    HdLighthouse2Mesh(SdfPath const& id, HdLighthouse2RenderDelegate* delegate);

    virtual ~HdLighthouse2Mesh();

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    virtual void Sync(HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits,
        TfToken const& reprToken) override;

    virtual void Finalize(HdRenderParam* renderParam) override;
    
protected:
    virtual void _InitRepr(TfToken const& reprToken,
        HdDirtyBits* dirtyBits) override;
    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

private:

    void _PopulateMesh(HdSceneDelegate* sceneDelegate,
        HdDirtyBits* dirtyBits,
        HdMeshReprDesc const& desc);

private:
    HdMeshTopology _topology;
    GfMatrix4f _transform;
    VtVec3fArray _points;
    VtVec3iArray _triangulatedIndices;
    VtIntArray _trianglePrimitiveParams;
    VtVec3fArray _computedNormals;
    Hd_VertexAdjacency _adjacency;
    bool _adjacencyValid;
    bool _normalsValid;
    bool _refined;
    bool _smoothNormals;
    bool _doubleSided;
    HdCullStyle _cullStyle;
    struct PrimvarSource {
        VtValue data;
        HdInterpolation interpolation;
    };
    TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> _primvarSourceMap;

    HdLighthouse2Mesh(const HdLighthouse2Mesh&) = delete;
    HdLighthouse2Mesh& operator =(const HdLighthouse2Mesh&) = delete;

    HdLighthouse2RenderDelegate* _owner;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
