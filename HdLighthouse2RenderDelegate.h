#ifndef HDLIGHTHOUSE2_RENDERDELEGATE_H
#define HDLIGHTHOUSE2_RENDERDELEGATE_H

#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/base/tf/staticTokens.h>

#include "platform.h"
#include "rendersystem.h"

#include <map>

PXR_NAMESPACE_USING_DIRECTIVE

using UpdateRenderSettingFunction = std::function<bool(pxr::VtValue const& value)>;

class HdLighthouse2RenderDelegate final : public pxr::HdRenderDelegate
{
public:
    HdLighthouse2RenderDelegate();
    HdLighthouse2RenderDelegate(pxr::HdRenderSettingsMap const& settingsMap);
    virtual ~HdLighthouse2RenderDelegate() override;
    
    HdLighthouse2RenderDelegate(const HdLighthouse2RenderDelegate&) = delete;
    HdLighthouse2RenderDelegate& operator=(const HdLighthouse2RenderDelegate&) = delete;

    virtual const pxr::TfTokenVector& GetSupportedRprimTypes() const override;
    virtual const pxr::TfTokenVector& GetSupportedSprimTypes() const override;
    virtual const pxr::TfTokenVector& GetSupportedBprimTypes() const override;

    virtual pxr::HdRenderParam* GetRenderParam() const override;
    //virtual HdRenderSettingDescriptorList GetRenderSettingDescriptors() const override;

    virtual pxr::HdAovDescriptor GetDefaultAovDescriptor(pxr::TfToken const& name) const override;

    virtual pxr::HdRprim* CreateRprim(pxr::TfToken const& typeId, pxr::SdfPath const& rprimId) override;
    virtual pxr::HdSprim* CreateSprim(pxr::TfToken const& typeId, pxr::SdfPath const& sprimId) override;
    virtual pxr::HdBprim* CreateBprim(pxr::TfToken const& typeId, pxr::SdfPath const& bprimId) override;

    virtual void DestroyRprim(pxr::HdRprim* rPrim) override;
    virtual void DestroySprim(pxr::HdSprim* sprim) override;
    virtual void DestroyBprim(pxr::HdBprim* bprim) override;

    virtual pxr::HdSprim* CreateFallbackSprim(pxr::TfToken const& typeId) override;
    virtual pxr::HdBprim* CreateFallbackBprim(pxr::TfToken const& typeId) override;

    virtual pxr::HdInstancer* CreateInstancer(pxr::HdSceneDelegate* delegate, pxr::SdfPath const& id) override;
    virtual void DestroyInstancer(pxr::HdInstancer* instancer) override;


    virtual pxr::HdRenderPassSharedPtr CreateRenderPass(pxr::HdRenderIndex* index, pxr::HdRprimCollection const& collection) override;

    virtual pxr::HdResourceRegistrySharedPtr GetResourceRegistry() const override;
    virtual void CommitResources(pxr::HdChangeTracker* tracker) override;
    virtual void SetRenderSetting(pxr::TfToken const& key, pxr::VtValue const& value) override;
    virtual pxr::VtValue GetRenderSetting(pxr::TfToken const& key) const override;

    RenderAPI* GetRenderer() { return _ltRenderer; }
    GLTexture* GetRenderTarget() { return _ltRenderTarget; }
    Shader* GetShader() { return _ltShader; }

    std::mutex& rendererMutex() { return _rendererMutex; }
    std::mutex& primIndexMutex() { return _primIndexMutex; }

    struct Lighthouse2Mesh {
        HostMesh* mesh;
        bool dirtyMesh = true;
        bool dirtyTransform = true;
        std::vector<int> indices;
        std::vector<float3> vertices;
        std::vector<float3> normals;
        std::vector<float2> uvs;
        std::vector<float2> uvs2;
        std::vector<float4> t;
        std::vector<HostMesh::Pose> poses;
        std::vector<uint4> joints;
        std::vector<float4> weights;
        std::vector<mat4> transforms;
        std::vector<int> instanceIDs;
        // facevarying primvars
        std::vector<float2> st;
    };

    struct Lighthouse2Material {
        HostMaterial* material;
    };

    void ResizeBuffer(int width, int height)
    {
        if( _ltRenderTarget )
            delete _ltRenderTarget;
        _ltRenderTarget = new GLTexture(width, height, GLTexture::FLOAT);
        _ltRenderer->SetTarget(_ltRenderTarget, 1);
    }

    Lighthouse2Mesh& GetMesh(const pxr::SdfPath& i_path)
    {
        if (_ltMeshes.find(i_path) == _ltMeshes.end())
        {
            _ltMeshes[i_path].mesh = new HostMesh();
        }
        return _ltMeshes[i_path];
    }

    Lighthouse2Mesh& GetLight(const pxr::SdfPath& i_path)
    {
        if (_ltLights.find(i_path) == _ltLights.end())
        {
            _ltLights[i_path].mesh = new HostMesh();
        }
        return _ltLights[i_path];
    }

    void BindMeshToMaterial(const pxr::SdfPath& i_mesh, const pxr::SdfPath& i_material)
    {
        _ltMeshToMaterialMap[i_mesh] = i_material;
    }

    Lighthouse2Material& GetMaterial(const pxr::SdfPath& i_path, float3 i_color=make_float3(1,1,1))
    {
        if (_ltMaterials.find(i_path) == _ltMaterials.end())
        {
            _ltMaterials[i_path].material = new HostMaterial();
            _ltMaterials[i_path].material->pbrtMaterialType = MaterialType::PBRT_DISNEY;
            _ltMaterials[i_path].material->color = i_color;
            //std::cout << "New material " << i_path << " color=" << i_color.x << "," << i_color.y << "," << i_color.z << std::endl;
            _ltMaterials[i_path].material->metallic = HostMaterial::ScalarValue(0.01f);
            _ltMaterials[i_path].material->roughness = HostMaterial::ScalarValue(0.5f);
            _ltMaterials[i_path].material->specular = HostMaterial::ScalarValue(0.01f);
        }
        return _ltMaterials[i_path];
    }

    bool UpdateScene();

    virtual pxr::TfToken GetMaterialBindingPurpose() const override;

#if HD_API_VERSION < 41
    virtual pxr::TfToken GetMaterialNetworkSelector() const override;
#else
    virtual pxr::TfTokenVector GetMaterialRenderContexts() const override;
#endif

    virtual pxr::TfTokenVector GetShaderSourceTypes() const override;

private:
    void _Initialize();

    static const pxr::TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const pxr::TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const pxr::TfTokenVector SUPPORTED_BPRIM_TYPES;

    static std::mutex _mutexResourceRegistry;
    static std::atomic_int _counterResourceRegistry;
    static HdResourceRegistrySharedPtr _resourceRegistry;

    std::mutex _rendererMutex;
    std::mutex _primIndexMutex;

    std::map<pxr::TfToken, UpdateRenderSettingFunction> _settingFunctions;

    static RenderAPI* _ltRenderer;
    static GLTexture* _ltRenderTarget;
    static Shader* _ltShader;
    static uint _ltCar;
    static std::map<pxr::SdfPath, Lighthouse2Mesh> _ltMeshes;
    static std::map<pxr::SdfPath, Lighthouse2Mesh> _ltLights; // area/rect lights
    static std::map<pxr::SdfPath, Lighthouse2Material> _ltMaterials;
    static std::map<pxr::SdfPath, pxr::SdfPath > _ltMeshToMaterialMap;
    static int _ltDefaultMaterial;

    pxr::HdRenderThread _renderThread;
};

#endif