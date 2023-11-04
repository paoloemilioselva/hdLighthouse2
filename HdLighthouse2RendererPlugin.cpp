#include "HdLighthouse2RendererPlugin.h"

#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include "HdLighthouse2RenderDelegate.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_REGISTRY_FUNCTION(TfType)
{
    pxr::HdRendererPluginRegistry::Define<HdLighthouse2RendererPlugin>();
}

pxr::HdRenderDelegate* HdLighthouse2RendererPlugin::CreateRenderDelegate()
{
    return new HdLighthouse2RenderDelegate();
}

pxr::HdRenderDelegate* HdLighthouse2RendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    return new HdLighthouse2RenderDelegate(settingsMap);
}

void HdLighthouse2RendererPlugin::DeleteRenderDelegate(pxr::HdRenderDelegate* renderDelegate)
{
    delete renderDelegate;
}

bool HdLighthouse2RendererPlugin::IsSupported() const
{
    return true;
}
