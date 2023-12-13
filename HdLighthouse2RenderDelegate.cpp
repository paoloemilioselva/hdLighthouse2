
#include <pxr/imaging/hd/driver.h>
#include <pxr/imaging/hd/camera.h>

#include "HdLighthouse2RenderDelegate.h"
#include "HdLighthouse2RenderPass.h"
#include "HdLighthouse2RenderBuffer.h"
#include "HdLighthouse2Mesh.h"
#include "HdLighthouse2Material.h"
#include "HdLighthouse2DomeLight.h"
#include "HdLighthouse2AreaLight.h"
#include "HdLighthouse2Camera.h"
#include "HdLighthouse2Instancer.h"

#include "Lighthouse2Utils.h"

#include <pxr/imaging/glf/glContext.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

std::mutex HdLighthouse2RenderDelegate::_mutexResourceRegistry;
std::atomic_int HdLighthouse2RenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr HdLighthouse2RenderDelegate::_resourceRegistry;

RenderAPI* HdLighthouse2RenderDelegate::_ltRenderer = nullptr;
GLTexture* HdLighthouse2RenderDelegate::_ltRenderTarget = nullptr;
Shader* HdLighthouse2RenderDelegate::_ltShader = nullptr;
uint HdLighthouse2RenderDelegate::_ltCar = 0;
std::map<pxr::SdfPath, HdLighthouse2RenderDelegate::Lighthouse2Mesh> HdLighthouse2RenderDelegate::_ltMeshes;
std::map<pxr::SdfPath, HdLighthouse2RenderDelegate::Lighthouse2Mesh> HdLighthouse2RenderDelegate::_ltLights;
std::map<pxr::SdfPath, HdLighthouse2RenderDelegate::Lighthouse2Material> HdLighthouse2RenderDelegate::_ltMaterials;
std::map<pxr::SdfPath, pxr::SdfPath > HdLighthouse2RenderDelegate::_ltMeshToMaterialMap;
int HdLighthouse2RenderDelegate::_ltDefaultMaterial;

static void _RenderCallback(RenderAPI* renderer, Shader* shader, GLTexture*  renderTarget, pxr::HdRenderThread* renderThread)
{
    //std::cout << "RENDERING" << std::endl;
    //renderer->SynchronizeSceneData();
    //renderer->Render(lighthouse2::Convergence::Restart /* alternative: converge */, true /*async*/);
}

const pxr::TfTokenVector HdLighthouse2RenderDelegate::SUPPORTED_RPRIM_TYPES = {
    HdPrimTypeTokens->mesh,
};

const pxr::TfTokenVector HdLighthouse2RenderDelegate::SUPPORTED_SPRIM_TYPES = {
    pxr::HdPrimTypeTokens->camera,
    pxr::HdPrimTypeTokens->material,
    pxr::HdPrimTypeTokens->domeLight,
    pxr::HdPrimTypeTokens->rectLight,
    //HdPrimTypeTokens->distantLight,
};

const pxr::TfTokenVector HdLighthouse2RenderDelegate::SUPPORTED_BPRIM_TYPES = {
    pxr::HdPrimTypeTokens->renderBuffer
};

pxr::TfTokenVector const& HdLighthouse2RenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

pxr::TfTokenVector const& HdLighthouse2RenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

pxr::TfTokenVector const& HdLighthouse2RenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdLighthouse2RenderDelegate::HdLighthouse2RenderDelegate()
    : HdRenderDelegate()
{
    _Initialize();
}

HdLighthouse2RenderDelegate::HdLighthouse2RenderDelegate(
    pxr::HdRenderSettingsMap const& settingsMap)
    : HdRenderDelegate(settingsMap)
{
    _Initialize();
}

void HdLighthouse2RenderDelegate::_Initialize()
{
    // THIS IS EXTREMELY IMPORTANT!!!!!
    // Otherwise all GL calls will fail!
    gladLoadGL();

    if (!_ltRenderer)
    {
        std::string ltPathStr = "";
        char* ltPath = getenv("LIGHTHOUSE2_PATH");
        if (ltPath)
        {
            ltPathStr = std::string(ltPath) + "/";
        }
        std::string vertPath = ltPathStr + "shaders/tonemap.vert";
        std::string fragPath = ltPathStr + "shaders/tonemap.frag";
        std::cout << "Lighthouse2 Loading shaders " << vertPath << " " << fragPath << std::endl;
        _ltShader = new Shader(vertPath.c_str(), fragPath.c_str());

        std::string corePath = ltPathStr + "Lighthouse2/lib/cores/RenderCore_Optix7.dll";
        std::cout << "Lighthouse2 Loading core DLL " << corePath << std::endl;
        _ltRenderer = RenderAPI::CreateRenderAPI(corePath.c_str());
        auto& mat = GetMaterial(pxr::SdfPath("_lighthouse2_default_material_"), make_float3(1, 1, 1));
        mat.material->ID = _ltRenderer->GetScene()->AddMaterial(mat.material);
        _ltDefaultMaterial = mat.material->ID;
    }

    if (!_ltRenderTarget)
    {
        // NOTE: this will be resized as soon as the renderer draws something
        //       so any value here is fine.
        ResizeBuffer(1024, 768);
    }

    _renderThread.SetRenderCallback(std::bind(_RenderCallback, _ltRenderer, _ltShader, _ltRenderTarget, &_renderThread));
    _renderThread.StartThread();

    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
    if (_counterResourceRegistry.fetch_add(1) == 0) {
        _resourceRegistry = std::make_shared<HdResourceRegistry>();
    }

}

HdLighthouse2RenderDelegate::~HdLighthouse2RenderDelegate()
{
    {
        std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
        if (_counterResourceRegistry.fetch_sub(1) == 1) {
            _resourceRegistry.reset();
        }
    }

    _renderThread.StopThread();
}



pxr::HdResourceRegistrySharedPtr HdLighthouse2RenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

void HdLighthouse2RenderDelegate::CommitResources(pxr::HdChangeTracker* /* tracker */)
{
    _resourceRegistry->Commit();
}

void HdLighthouse2RenderDelegate::SetRenderSetting(pxr::TfToken const& key, pxr::VtValue const& value)
{
}

pxr::VtValue HdLighthouse2RenderDelegate::GetRenderSetting(pxr::TfToken const& key) const
{
    return HdRenderDelegate::GetRenderSetting(key);
}

pxr::HdRenderPassSharedPtr HdLighthouse2RenderDelegate::CreateRenderPass(
    pxr::HdRenderIndex* index,
    pxr::HdRprimCollection const& collection)
{
    return pxr::HdRenderPassSharedPtr(new HdLighthouse2RenderPass(index, collection, &_renderThread, this));
}

pxr::HdRprim* HdLighthouse2RenderDelegate::CreateRprim(pxr::TfToken const& typeId, pxr::SdfPath const& rprimId)
{
    if (typeId == HdPrimTypeTokens->mesh) 
    {
        return new HdLighthouse2Mesh(rprimId, this);
    }
    return nullptr;
}

void HdLighthouse2RenderDelegate::DestroyRprim(pxr::HdRprim* rPrim)
{
    delete rPrim;
}

pxr::HdSprim* HdLighthouse2RenderDelegate::CreateSprim(pxr::TfToken const& typeId, pxr::SdfPath const& sprimId)
{
    if (typeId == pxr::HdPrimTypeTokens->camera)
    {
        return new HdLighthouse2Camera(sprimId, this);
    }
    else if (typeId == HdPrimTypeTokens->material)
    {
        return new HdLighthouse2Material(sprimId, this);
    }
    else if (typeId == HdPrimTypeTokens->domeLight)
    {
        return new HdLighthouse2DomeLight(sprimId, this);
    }
    else if (typeId == HdPrimTypeTokens->rectLight)
    {
        return new HdLighthouse2AreaLight(sprimId, this);
    }
    //else if (typeId == HdPrimTypeTokens->distantLight)
    //{
    //    return new HdAuroraDistantLight(sprimId, this);
    //}
    return nullptr;
}

pxr::HdSprim* HdLighthouse2RenderDelegate::CreateFallbackSprim(pxr::TfToken const& typeId)
{
    if (typeId == pxr::HdPrimTypeTokens->camera)
    {
        return new HdLighthouse2Camera(pxr::SdfPath::EmptyPath(), this);
    }
    else if (typeId == HdPrimTypeTokens->material)
    {
        return new HdLighthouse2Material(pxr::SdfPath::EmptyPath(), this);
    }
    else if (typeId == HdPrimTypeTokens->domeLight)
    {
        return new HdLighthouse2DomeLight(pxr::SdfPath::EmptyPath(), this);
    }
    else if (typeId == HdPrimTypeTokens->rectLight)
    {
        return new HdLighthouse2AreaLight(pxr::SdfPath::EmptyPath(), this);
    }

    return nullptr;
}

void HdLighthouse2RenderDelegate::DestroySprim(pxr::HdSprim* sPrim)
{
    delete sPrim;
}

pxr::HdBprim* HdLighthouse2RenderDelegate::CreateBprim(pxr::TfToken const& typeId, pxr::SdfPath const& bprimId)
{
    if (typeId == pxr::HdPrimTypeTokens->renderBuffer)
    {
        return new HdLighthouse2RenderBuffer(bprimId);
    }
    return nullptr;
}

pxr::HdBprim* HdLighthouse2RenderDelegate::CreateFallbackBprim(pxr::TfToken const& typeId)
{    
    return nullptr;
}

void HdLighthouse2RenderDelegate::DestroyBprim(pxr::HdBprim* bPrim)
{
}

pxr::HdInstancer* HdLighthouse2RenderDelegate::CreateInstancer(pxr::HdSceneDelegate* delegate, pxr::SdfPath const& id)
{
    return new HdLighthouse2Instancer(delegate, id);
}

void HdLighthouse2RenderDelegate::DestroyInstancer(pxr::HdInstancer* instancer)
{
    delete instancer;
}

pxr::HdRenderParam* HdLighthouse2RenderDelegate::GetRenderParam() const
{
    return nullptr;
}

pxr::HdAovDescriptor HdLighthouse2RenderDelegate::GetDefaultAovDescriptor(pxr::TfToken const& name) const
{
    if (name == pxr::HdAovTokens->color)
    {
        return pxr::HdAovDescriptor(pxr::HdFormatFloat16Vec4, false, pxr::VtValue(pxr::GfVec4f(0.0f)));
    }

    return pxr::HdAovDescriptor(pxr::HdFormatInvalid, false, pxr::VtValue());
}

bool HdLighthouse2RenderDelegate::UpdateScene()
{
    bool result = false;

    // check for new materials before checking for meshes
    //
    for (auto it = _ltMaterials.begin(); it != _ltMaterials.end(); ++it)
    {
        if (it->second.material->ID == -1)
        {
            it->second.material->ID = _ltRenderer->GetScene()->AddMaterial(it->second.material);
        }
    }

    // set skydome
    // 
    //if (_ltRenderer->GetScene()->sky == nullptr && _ltLights.size() == 0)
    //{
    //    std::string ltPathStr = "";
    //    char* ltPath = getenv("LIGHTHOUSE2_PATH");
    //    if (ltPath)
    //    {
    //        ltPathStr = std::string(ltPath) + "/";
    //    }
    //    std::string skyPath = ltPathStr + "ibls/qwantani_puresky_2k.hdr";

    //    _ltRenderer->GetScene()->sky = new HostSkyDome();
    //    _ltRenderer->GetScene()->sky->Load(skyPath.c_str());
    //    _ltRenderer->GetScene()->sky->worldToLight = mat4::RotateX(-PI / 2); // compensate for different evaluation in PBRT
    //}

    // check for new meshes
    //
    for (auto it = _ltMeshes.begin(); it != _ltMeshes.end(); ++it)
    {
        if (it->second.dirtyMesh || it->second.dirtyTransform)
        {
            result = true;
        }

        if (it->second.mesh->ID != -1 && it->second.dirtyMesh)
        {
            it->second.dirtyMesh = false;
        }


        if (it->second.mesh->ID == -1)
        {
            it->second.dirtyMesh = false;

            //std::cout << "Building mesh for " << it->first << std::endl;
            _ltRenderer->GetScene()->AddMesh(it->second.mesh);
            
            auto matId = _ltDefaultMaterial;
            if (_ltMeshToMaterialMap.find(it->first) != _ltMeshToMaterialMap.end())
            {
                auto& matPath = _ltMeshToMaterialMap[it->first];
                if (_ltMaterials.find(matPath) != _ltMaterials.end())
                {
                    matId = _ltMaterials[matPath].material->ID;
                }
            }

            if (it->second.st.size() == 0)
            {
                it->second.mesh->BuildFromIndexedData(
                    it->second.indices,
                    it->second.vertices,
                    it->second.normals,
                    it->second.uvs,
                    it->second.uvs2,
                    it->second.t,
                    it->second.poses,
                    it->second.joints,
                    it->second.weights,
                    matId
                );
            }
            // Add facevarying primvars
            else
            {
                Lighthouse2Utils::BuildFromIndexedData(
                    it->second.mesh,
                    it->second.indices,
                    it->second.vertices,
                    it->second.normals,
                    it->second.st,
                    matId
                );
            }
            //it->second.mesh->BuildMaterialList();
            it->second.instanceIDs.resize(it->second.transforms.size());
            for (int i = 0; i < it->second.transforms.size(); ++i)
                it->second.instanceIDs[i] = _ltRenderer->AddInstance(it->second.mesh->ID, it->second.transforms[i]);
        }
        
        if (it->second.dirtyTransform)
        {
            it->second.dirtyTransform = false;
            for (int i = 0; i < it->second.transforms.size(); ++i)
                _ltRenderer->SetNodeTransform(it->second.instanceIDs[i], it->second.transforms[i]);
        }
    }

    // check for new lights
    //
    for (auto it = _ltLights.begin(); it != _ltLights.end(); ++it)
    {
        if (it->second.dirtyMesh || it->second.dirtyTransform)
        {
            result = true;
        }

        if (it->second.mesh->ID != -1 && it->second.dirtyMesh)
        {
            it->second.dirtyMesh = false;
            Lighthouse2Utils::UpdateVertices(
                it->second.mesh,
                it->second.indices,
                it->second.vertices );
            //_ltRenderer->GetScene()->nodePool[it->second.mesh->ID]->PrepareLights();
            //_ltRenderer->GetScene()->nodePool[it->second.mesh->ID]->UpdateLights();
            _ltRenderer->GetScene()->nodePool[it->second.mesh->ID]->treeChanged = true;
            // dirty mesh, delete and recreate
            // TODO: figure out if we can update vertices in Lighthouse2
            //for (int i = 0; i < it->second.instanceIDs.size(); ++i)
            //    _ltRenderer->GetScene()->RemoveNode(it->second.instanceIDs[i]);
            //it->second.instanceIDs.resize(it->second.transforms.size());
            //for (int i = 0; i < it->second.transforms.size(); ++i)
            //    it->second.instanceIDs[i] = _ltRenderer->AddInstance(it->second.mesh->ID, it->second.transforms[i]);
            //it->second.mesh->ID = -1;
        }


        if (it->second.mesh->ID == -1)
        {
            it->second.dirtyMesh = false;
            _ltRenderer->GetScene()->AddMesh(it->second.mesh);

            auto matId = _ltDefaultMaterial;
            if (_ltMeshToMaterialMap.find(it->first) != _ltMeshToMaterialMap.end())
            {
                auto& matPath = _ltMeshToMaterialMap[it->first];
                if (_ltMaterials.find(matPath) != _ltMaterials.end())
                {
                    matId = _ltMaterials[matPath].material->ID;
                }
            }

            if (it->second.st.size() == 0)
            {
                it->second.mesh->BuildFromIndexedData(
                    it->second.indices,
                    it->second.vertices,
                    it->second.normals,
                    it->second.uvs,
                    it->second.uvs2,
                    it->second.t,
                    it->second.poses,
                    it->second.joints,
                    it->second.weights,
                    matId
                );
            }
            // Add facevarying primvars
            else
            {
                Lighthouse2Utils::BuildFromIndexedData(
                    it->second.mesh,
                    it->second.indices,
                    it->second.vertices,
                    it->second.normals,
                    it->second.st,
                    matId
                );
            }
            it->second.instanceIDs.resize(it->second.transforms.size());
            for (int i = 0; i < it->second.transforms.size(); ++i)
                it->second.instanceIDs[i] = _ltRenderer->AddInstance(it->second.mesh->ID, it->second.transforms[i]);
        }

        if (it->second.dirtyTransform)
        {
            it->second.dirtyTransform = false;
            for (int i = 0; i < it->second.transforms.size(); ++i)
                _ltRenderer->SetNodeTransform(it->second.instanceIDs[i], it->second.transforms[i]);
        }
    }

    //static float r = 0;
    //mat4 M = mat4::RotateY(r * 2.0f) * mat4::RotateZ(0.2f * sinf(r * 8.0f)) * mat4::Translate(make_float3(0, 5, 0));
    //_ltRenderer->SetNodeTransform(_ltCar, M);
    //if ((r += 0.025f * 0.3f) > 2 * PI) r -= 2 * PI;

    return result;
}

pxr::TfToken HdLighthouse2RenderDelegate::GetMaterialBindingPurpose() const
{
    return HdTokens->full;
}

#if HD_API_VERSION < 41
pxr::TfToken HdLighthouse2RenderDelegate::GetMaterialNetworkSelector() const
{
    static const pxr::TfToken mtlx("mtlx");
    return mtlx;
}
#else
pxr::TfTokenVector HdLighthouse2RenderDelegate::GetMaterialRenderContexts() const
{
    static const pxr::TfToken mtlx("mtlx");
    static const pxr::TfToken lighthouse2("lt2");
    return { lighthouse2, mtlx };
}
#endif

pxr::TfTokenVector HdLighthouse2RenderDelegate::GetShaderSourceTypes() const
{
    return HdLighthouse2Material::GetShaderSourceTypes();
}
