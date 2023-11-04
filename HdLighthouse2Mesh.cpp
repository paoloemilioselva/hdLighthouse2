#include "HdLighthouse2Mesh.h"
#include "HdLighthouse2RenderPass.h"
#include "HdLighthouse2Instancer.h"
#include <pxr/imaging/hd/extComputationUtils.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/vertexAdjacency.h>
#include <pxr/imaging/hd/smoothNormals.h>
#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/pxOsd/tokens.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <algorithm> // sort

PXR_NAMESPACE_OPEN_SCOPE

HdLighthouse2Mesh::HdLighthouse2Mesh(SdfPath const& id, HdLighthouse2RenderDelegate* delegate)
    : HdMesh(id)
    , _adjacencyValid(false)
    , _normalsValid(false)
    , _refined(false)
    , _smoothNormals(false)
    , _doubleSided(false)
    , _cullStyle(HdCullStyleDontCare)
    , _owner(delegate)
{
}

void
HdLighthouse2Mesh::Finalize(HdRenderParam* renderParam)
{
}

HdDirtyBits
HdLighthouse2Mesh::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllSceneDirtyBits;
}

HdDirtyBits
HdLighthouse2Mesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void
HdLighthouse2Mesh::_InitRepr(TfToken const& reprToken,
    HdDirtyBits* dirtyBits)
{
    TF_UNUSED(dirtyBits);
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
        _ReprComparator(reprToken));
    if (it == _reprs.end()) {
        _reprs.emplace_back(reprToken, HdReprSharedPtr());
    }
}

void
HdLighthouse2Mesh::Sync(HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    TfToken const& reprToken)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
    const HdMeshReprDesc& desc = descs[0];

    _PopulateMesh(sceneDelegate, dirtyBits, desc);
}

void
HdLighthouse2Mesh::_PopulateMesh(HdSceneDelegate* sceneDelegate,
    HdDirtyBits* dirtyBits,
    HdMeshReprDesc const& desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    //std::cout << "Syncing " << id << std::endl;

    // Synchronize instancer.
    _UpdateInstancer(sceneDelegate, dirtyBits);
    HdInstancer::_SyncInstancerAndParents(sceneDelegate->GetRenderIndex(), GetInstancerId());


    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        VtValue value = sceneDelegate->Get(id, HdTokens->points);
        _points = value.Get<VtVec3fArray>();
        _normalsValid = false;
    }

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        PxOsdSubdivTags subdivTags = _topology.GetSubdivTags();
        int refineLevel = _topology.GetRefineLevel();
        _topology = HdMeshTopology(GetMeshTopology(sceneDelegate), refineLevel);
        _topology.SetSubdivTags(subdivTags);
        _adjacencyValid = false;
    }
    if (HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id) &&
        _topology.GetRefineLevel() > 0) {
        _topology.SetSubdivTags(sceneDelegate->GetSubdivTags(id));
    }
    if (HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {
        HdDisplayStyle const displayStyle = sceneDelegate->GetDisplayStyle(id);
        _topology = HdMeshTopology(_topology,
            displayStyle.refineLevel);
    }

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        _UpdateVisibility(sceneDelegate, dirtyBits);
    }

    if (HdChangeTracker::IsCullStyleDirty(*dirtyBits, id)) {
        _cullStyle = GetCullStyle(sceneDelegate);
    }
    if (HdChangeTracker::IsDoubleSidedDirty(*dirtyBits, id)) {
        _doubleSided = IsDoubleSided(sceneDelegate);
    }
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->widths) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar)) {
        //_UpdatePrimvarSources(sceneDelegate, *dirtyBits);
    }

    bool _materialChanged = false;
    auto matId = sceneDelegate->GetMaterialId(id);
    if (matId != GetMaterialId())
    {
        SetMaterialId(matId);
        _materialChanged = true;
    }


    bool doRefine = (desc.geomStyle == HdMeshGeomStyleSurf);
    doRefine = doRefine && (_topology.GetScheme() != PxOsdOpenSubdivTokens->none);
    doRefine = doRefine && (_topology.GetRefineLevel() > 0);
    _smoothNormals = !desc.flatShadingEnabled;
    _smoothNormals = _smoothNormals &&
        (_topology.GetScheme() != PxOsdOpenSubdivTokens->none) &&
        (_topology.GetScheme() != PxOsdOpenSubdivTokens->bilinear);
    bool authoredNormals = false;
    if (_primvarSourceMap.count(HdTokens->normals) > 0) {
        authoredNormals = true;
    }
    _smoothNormals = _smoothNormals && !authoredNormals;

    bool newMesh = false;
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id) || doRefine != _refined) 
    {
        newMesh = true;

        // Destroy the old mesh, if it exists.

        // Create the prototype mesh scene, if it doesn't exist yet.
        //_owner->GetRenderer()->WaitForRender();


        _refined = doRefine;
        // In both cases, RTC_VERTEX_BUFFER will be populated below.

        //// Prototype geometry gets tagged with a prototype context, that the
        //// ray-hit algorithm can use to look up data.
        //rtcSetGeometryUserData(_geometry, new HdByoPrototypeContext);
        //_GetPrototypeContext()->rprim = this;
        //_GetPrototypeContext()->primitiveParams = (_refined ?
        //    _trianglePrimitiveParams : VtIntArray());

        //// Add _EmbreeCullFaces as a filter function for backface culling.
        //rtcSetGeometryIntersectFilterFunction(_geometry, _EmbreeCullFaces);
        //rtcSetGeometryOccludedFilterFunction(_geometry, _EmbreeCullFaces);

        // Force the smooth normals code to rebuild the "normals" primvar the
        // next time smooth normals is enabled.
        _normalsValid = false;
    }

 
    // Populate points in the RTC mesh.
    if (newMesh || HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) 
    {

        std::lock_guard<std::mutex> guard(_owner->rendererMutex());
        auto& mesh = _owner->GetMesh(id);

        // check for uv(for vertex-varying) or st(for facevarying)
        VtArray<GfVec2f> st;
        VtArray<GfVec2f> uvs;

        _topology = sceneDelegate->GetMeshTopology(id);
        HdMeshUtil meshUtil(&_topology, id);
        VtIntArray trianglePrimitiveParams;
        meshUtil.ComputeTriangleIndices(&_triangulatedIndices, &trianglePrimitiveParams);

        // check for primvars of interest
        // 
        std::vector<HdInterpolation> primvarInterpolations = {
            HdInterpolation::HdInterpolationVertex,
            HdInterpolation::HdInterpolationFaceVarying,
        };
        for (auto& primvarInterpolation : primvarInterpolations)
        {
            HdPrimvarDescriptorVector pvs = sceneDelegate->GetPrimvarDescriptors(id, primvarInterpolation);
            for (auto& pv : pvs)
            {
                // Check for UV/STs
                //
                if (pv.name == UsdUtilsGetPrimaryUVSetName())
                {
                    if (primvarInterpolation == HdInterpolation::HdInterpolationVertex)
                    {
                        const VtValue& stVal = sceneDelegate->Get(id, pv.name);
                        if (stVal.GetArraySize() == _points.size())
                        {
                            // valid vertex st
                            uvs = stVal.Get<VtArray<GfVec2f>>();
                        }
                    }
                    else
                    {
                        // Get primvar for STs
                        VtValue stVal;
                        auto tmpSt = GetPrimvar(sceneDelegate, pv.name);
                        HdVtBufferSource buffer(pv.name, tmpSt);
                        // Triangulate the STs (should produce an ST for each index)
                        meshUtil.ComputeTriangulatedFaceVaryingPrimvar(
                            buffer.GetData(), (int)buffer.GetNumElements(), buffer.GetTupleType().type, &stVal);
                        st = stVal.Get<VtArray<GfVec2f>>();
                        for (auto& c : st)
                        {
                            mesh.st.emplace_back(make_float2(c.data()[0], c.data()[1]));
                        }
                    }
                }
                // any other primvar ?
            }
        }

        // Make sure we have all triangles
        mesh.indices.clear();

        for (auto& t : _triangulatedIndices)
        {
            mesh.indices.emplace_back(t.data()[0]);
            mesh.indices.emplace_back(t.data()[1]);
            mesh.indices.emplace_back(t.data()[2]);
        }

        // Get normals (smooth them for now)
        //
        //std::cout << "Building normals..." << std::endl;
        auto& normals = sceneDelegate->Get(id, pxr::HdTokens->normals).Get<pxr::VtVec3fArray>();
        Hd_VertexAdjacency _adjacency;
        _adjacency.BuildAdjacencyTable(&_topology);
        auto _computedNormals = Hd_SmoothNormals::ComputeSmoothNormals(&_adjacency, _points.size(), _points.cdata());

        // search for displayColor, but only use first value for now
        //
        //std::cout << "Building displayColors..." << std::endl;
        float3 displayColor = make_float3(1, 1, 1);

        if (matId.IsEmpty())
        {
            matId = id;
            VtValue colorValue = sceneDelegate->Get(id, HdTokens->displayColor);
            if (!colorValue.IsEmpty() && colorValue.IsHolding<VtVec3fArray>())
            {
                auto& valueArray = colorValue.Get<VtVec3fArray>();
                displayColor = make_float3( 
                    valueArray[0].data()[0], 
                    valueArray[0].data()[1], 
                    valueArray[0].data()[2]);
            }
        }
        auto& mat = _owner->GetMaterial(matId, displayColor);
        _owner->BindMeshToMaterial(id, matId);

        for (int pi = 0; pi < _points.size();++pi)
        {
            mesh.vertices.emplace_back(make_float3(_points[pi].data()[0], _points[pi].data()[1], _points[pi].data()[2]));
            mesh.normals.emplace_back(make_float3(_computedNormals[pi].data()[0], _computedNormals[pi].data()[1], _computedNormals[pi].data()[2]));
            if (uvs.size())
            {
                mesh.uvs.emplace_back(make_float2(uvs[pi].data()[0], uvs[pi].data()[1]));
            }
        }

    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) 
    {
        _transform = GfMatrix4f(sceneDelegate->GetTransform(id));

        std::lock_guard<std::mutex> guard(_owner->rendererMutex());
        auto& mesh = _owner->GetMesh(id);
        mesh.dirtyTransform = true;

        if (!GetInstancerId().IsEmpty())
        {
            const auto& transposed = _transform.GetTranspose();

            // retrieve instance transforms from the instancer.
            VtMatrix4dArray transforms;
            HdInstancer* instancer = sceneDelegate->GetRenderIndex().GetInstancer(GetInstancerId());
            transforms = static_cast<HdLighthouse2Instancer*>(instancer)->ComputeInstanceTransforms(GetId());

            // apply instance transforms on top of mesh transforms
            if( transforms.size() != mesh.transforms.size() )
                mesh.transforms.resize(transforms.size());

            for (size_t j = 0; j < transforms.size(); ++j)
            {
                GfMatrix4f xform = (_transform * GfMatrix4f(transforms[j])).GetTranspose();
                for (int i = 0; i < 16; ++i)
                {
                    mesh.transforms[j].cell[i] = xform.data()[i];
                }
                //mesh.transforms[j].path = GetId().GetString() + "_Instance" + std::to_string(j);
                //mesh.transforms[j].properties[Aurora::Names::InstanceProperties::kTransform] =
                //    GfMatrix4fToGLM(&xform);
            }
        }
        else
        {
            const auto& transposed = _transform.GetTranspose();
            if( mesh.transforms.size() != 1 )
                mesh.transforms.resize(1);
            for (int i = 0; i < 16; ++i)
            {
                mesh.transforms[0].cell[i] = transposed.data()[i];
            }
        }

    }


    // Clean all dirty bits.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}


PXR_NAMESPACE_CLOSE_SCOPE
