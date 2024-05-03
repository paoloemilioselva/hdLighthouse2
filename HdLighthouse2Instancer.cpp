
#include "HdLighthouse2Instancer.h"
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/base/gf/quath.h>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE


HdLighthouse2Instancer::HdLighthouse2Instancer(pxr::HdSceneDelegate* delegate, pxr::SdfPath const& id) :
    pxr::HdInstancer(delegate, id)
{
}

HdLighthouse2Instancer::~HdLighthouse2Instancer()
{
    TF_FOR_ALL(it, _primvarMap)
    {
        delete it->second;
    }
    _primvarMap.clear();
}

void HdLighthouse2Instancer::Sync(
    pxr::HdSceneDelegate* delegate, pxr::HdRenderParam* /*renderParam*/, pxr::HdDirtyBits* dirtyBits)
{
    _UpdateInstancer(delegate, dirtyBits);

    if (pxr::HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, GetId()))
    {
        _SyncPrimvars(delegate, *dirtyBits);
    }
}

void HdLighthouse2Instancer::_SyncPrimvars(pxr::HdSceneDelegate* delegate, pxr::HdDirtyBits dirtyBits)
{
    pxr::SdfPath const& id = GetId();

    pxr::HdPrimvarDescriptorVector primvars =
        delegate->GetPrimvarDescriptors(id, pxr::HdInterpolationInstance);

    for (pxr::HdPrimvarDescriptor const& pv : primvars)
    {
        //if (pxr::HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name))
        {
            pxr::VtValue value = delegate->Get(id, pv.name);
            if (!value.IsEmpty())
            {
                if (_primvarMap.count(pv.name) > 0)
                {
                    delete _primvarMap[pv.name];
                }
                _primvarMap[pv.name] = new pxr::HdVtBufferSource(pv.name, value);
            }
        }
    }
}

bool SampleBuffer(pxr::HdVtBufferSource const& buffer, int index, void* value, pxr::HdTupleType dataType)
{
    if (buffer.GetNumElements() <= (size_t)index || buffer.GetTupleType() != dataType)
    {
        return false;
    }

    size_t elemSize = pxr::HdDataSizeOfTupleType(dataType);
    size_t offset = elemSize * index;

    std::memcpy(value, static_cast<const uint8_t*>(buffer.GetData()) + offset, elemSize);

    return true;
}

pxr::VtMatrix4dArray HdLighthouse2Instancer::ComputeInstanceTransforms(pxr::SdfPath const& prototypeId)
{
    // The transforms for this level of instancer are computed by:
    // foreach(index : indices) {
    //     instancerTransform * translate(index) * rotate(index) *
    //     scale(index) * instanceTransform(index)
    // }
    // If any transform isn't provided, it's assumed to be the identity.

    pxr::GfMatrix4d instancerTransform = GetDelegate()->GetInstancerTransform(GetId());
    pxr::VtIntArray instanceIndices = GetDelegate()->GetInstanceIndices(GetId(), prototypeId);

    pxr::VtMatrix4dArray transforms(instanceIndices.size());
    for (size_t i = 0; i < instanceIndices.size(); ++i)
    {
        transforms[i] = instancerTransform;
    }
    pxr::HdVtBufferSource* pBuffer = nullptr;

    if (_primvarMap.count(pxr::HdInstancerTokens->instanceTranslations) > 0)
        pBuffer = _primvarMap[pxr::HdInstancerTokens->instanceTranslations];
    else if (_primvarMap.count(pxr::HdInstancerTokens->translate) > 0)
        pBuffer = _primvarMap[pxr::HdInstancerTokens->translate];
    if (pBuffer)
    {
        for (size_t i = 0; i < instanceIndices.size(); ++i)
        {
            pxr::GfVec3f translate;
            if (SampleBuffer(*pBuffer, instanceIndices[i], &translate, { pxr::HdTypeFloatVec3, 1 }))
            {
                pxr::GfMatrix4d translateMat(1);
                translateMat.SetTranslate(pxr::GfVec3d(translate));
                transforms[i] = translateMat * transforms[i];
            }
        }
    }

    if (_primvarMap.count(pxr::HdInstancerTokens->instanceRotations) > 0)
        pBuffer = _primvarMap[pxr::HdInstancerTokens->instanceRotations];
    else if (_primvarMap.count(pxr::HdInstancerTokens->rotate) > 0)
        pBuffer = _primvarMap[pxr::HdInstancerTokens->rotate];
    if (pBuffer)
    {
        for (size_t i = 0; i < instanceIndices.size(); ++i)
        {
            pxr::GfQuath quat;
            if (SampleBuffer(*pBuffer, instanceIndices[i], &quat, { pxr::HdTypeHalfFloatVec4, 1 }))
            {
                pxr::GfMatrix4d rotateMat(1);
                rotateMat.SetRotate(quat);
                transforms[i] = rotateMat * transforms[i];
            }
            else
            {
                pxr::GfVec4f quatf;
                if (SampleBuffer(*pBuffer, instanceIndices[i], &quatf, { pxr::HdTypeFloatVec4, 1 }))
                {
                    pxr::GfMatrix4d rotateMat(1);
                    rotateMat.SetRotate(pxr::GfQuatd(quatf[0], quatf[1], quatf[2], quatf[3]));
                    transforms[i] = rotateMat * transforms[i];
                }
            }
        }
    }

    if (_primvarMap.count(pxr::HdInstancerTokens->instanceScales) > 0)
        pBuffer = _primvarMap[pxr::HdInstancerTokens->instanceScales];
    else if (_primvarMap.count(pxr::HdInstancerTokens->scale) > 0)
        pBuffer = _primvarMap[pxr::HdInstancerTokens->scale];
    if (pBuffer)
    {
        for (size_t i = 0; i < instanceIndices.size(); ++i)
        {
            pxr::GfVec3f scale;
            if (SampleBuffer(*pBuffer, instanceIndices[i], &scale, { pxr::HdTypeFloatVec3, 1 }))
            {
                pxr::GfMatrix4d scaleMat(1);
                scaleMat.SetScale(pxr::GfVec3d(scale));
                transforms[i] = scaleMat * transforms[i];
            }
        }
    }

    if (_primvarMap.count(pxr::HdInstancerTokens->instanceTransforms) > 0)
        pBuffer = _primvarMap[pxr::HdInstancerTokens->instanceTransforms];
    else if (_primvarMap.count(pxr::HdInstancerTokens->instanceTransform) > 0)
        pBuffer = _primvarMap[pxr::HdInstancerTokens->instanceTransform];
    if (pBuffer)
    {
        for (size_t i = 0; i < instanceIndices.size(); ++i)
        {
            pxr::GfMatrix4d instanceTransform;
            if (SampleBuffer(
                *pBuffer, instanceIndices[i], &instanceTransform, { pxr::HdTypeDoubleMat4, 1 }))
            {
                transforms[i] = instanceTransform * transforms[i];
            }
        }
    }

    if (GetParentId().IsEmpty())
    {
        return transforms;
    }

    HdInstancer* parentInstancer = GetDelegate()->GetRenderIndex().GetInstancer(GetParentId());
    if (!parentInstancer)
    {
        return transforms;
    }

    // The transforms taking nesting into account are computed by:
    // parentTransforms = parentInstancer->ComputeInstanceTransforms(GetId())
    // foreach (parentXf : parentTransforms, xf : transforms) {
    //     parentXf * xf
    // }
    pxr::VtMatrix4dArray parentTransforms =
        static_cast<HdLighthouse2Instancer*>(parentInstancer)->ComputeInstanceTransforms(GetId());

    pxr::VtMatrix4dArray final(parentTransforms.size() * transforms.size());
    for (size_t i = 0; i < parentTransforms.size(); ++i)
    {
        for (size_t j = 0; j < transforms.size(); ++j)
        {
            final[i * transforms.size() + j] = transforms[j] * parentTransforms[i];
        }
    }
    return final;
}