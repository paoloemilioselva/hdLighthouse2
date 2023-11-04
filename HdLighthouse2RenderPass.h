#ifndef HDLIGHTHOUSE2_RENDERPASS_H
#define HDLIGHTHOUSE2_RENDERPASS_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderPass.h>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/rect2i.h>

#include "HdLighthouse2RenderBuffer.h"
#include "HdLighthouse2RenderDelegate.h"

class HdLighthouse2RenderPass final : public pxr::HdRenderPass
{
public:
    HdLighthouse2RenderPass(
        pxr::HdRenderIndex* index, 
        pxr::HdRprimCollection const& collection,
        pxr::HdRenderThread* renderThread,
        HdLighthouse2RenderDelegate* renderDelegate);

    ~HdLighthouse2RenderPass();

    bool IsConverged() const override;

protected:
    void _Execute(
        pxr::HdRenderPassStateSharedPtr const& renderPassState,
        pxr::TfTokenVector const& renderTags) override;

    void _MarkCollectionDirty() override {}

private:
    HdLighthouse2RenderDelegate* _owner;

    pxr::HdRenderPassAovBindingVector _aovBindings;
    pxr::GfRect2i _dataWindow;
    pxr::GfMatrix4d _viewMatrix;
    pxr::GfMatrix4d _projMatrix;
    HdLighthouse2RenderBuffer _colorBuffer;
    pxr::HdRenderThread* _renderThread;
};

#endif