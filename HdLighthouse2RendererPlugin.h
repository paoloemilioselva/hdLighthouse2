#ifndef HDLIGHTHOUSE2_RENDERERPLUGIN_H
#define HDLIGHTHOUSE2_RENDERERPLUGIN_H

#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/pxr.h>

class HdLighthouse2RendererPlugin final : public pxr::HdRendererPlugin
{
public:
    HdLighthouse2RendererPlugin() = default;
    virtual ~HdLighthouse2RendererPlugin() = default;

    pxr::HdRenderDelegate* CreateRenderDelegate() override;
    pxr::HdRenderDelegate* CreateRenderDelegate(pxr::HdRenderSettingsMap const& settingsMap) override;

    void DeleteRenderDelegate(pxr::HdRenderDelegate* renderDelegate) override;

#if PXR_VERSION > 2205
    bool IsSupported(bool) const override;
#else
    bool IsSupported() const override;
#endif

private:
    HdLighthouse2RendererPlugin(const HdLighthouse2RendererPlugin&) = delete;
    HdLighthouse2RendererPlugin& operator =(const HdLighthouse2RendererPlugin&) = delete;

};

#endif
