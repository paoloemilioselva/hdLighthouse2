#include "HdLighthouse2RenderPass.h"
#include "HdLighthouse2RenderBuffer.h"
#include "HdLighthouse2RenderDelegate.h"
#include "HdLighthouse2Camera.h"

#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/glf/glContext.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/quaternion.h>
#include <pxr/base/gf/matrix3d.h>

#include <iostream>
#include <bitset>

HdLighthouse2RenderPass::HdLighthouse2RenderPass(
    pxr::HdRenderIndex* index, 
    pxr::HdRprimCollection const& collection,
    pxr::HdRenderThread* renderThread,
    HdLighthouse2RenderDelegate* renderDelegate) 
    : pxr::HdRenderPass(index, collection)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _aovBindings()
    , _colorBuffer(SdfPath::EmptyPath())
    , _renderThread(renderThread)
    , _owner(renderDelegate)
{
}

HdLighthouse2RenderPass::~HdLighthouse2RenderPass()
{
}

bool HdLighthouse2RenderPass::IsConverged() const
{
    if (_aovBindings.size() == 0) 
        return true;

    for (size_t i = 0; i < _aovBindings.size(); ++i) 
        if (_aovBindings[i].renderBuffer && !_aovBindings[i].renderBuffer->IsConverged()) 
            return false;
    return true;
}

static pxr::GfRect2i _GetDataWindow(pxr::HdRenderPassStateSharedPtr const& renderPassState)
{
    const pxr::CameraUtilFraming& framing = renderPassState->GetFraming();
    if (framing.IsValid()) {
        return framing.dataWindow;
    }
    else {
        const pxr::GfVec4f vp = renderPassState->GetViewport();
        return pxr::GfRect2i(pxr::GfVec2i(0), int(vp[2]), int(vp[3]));
    }
}

void HdLighthouse2RenderPass::_Execute(
    pxr::HdRenderPassStateSharedPtr const& renderPassState,
    pxr::TfTokenVector const& renderTags)
{
    bool needStartRender = false;

    // has the camera moved ?
    //
    const pxr::GfMatrix4d view = renderPassState->GetWorldToViewMatrix();
    const pxr::GfMatrix4d proj = renderPassState->GetProjectionMatrix();

    if (_viewMatrix != view || _projMatrix != proj) 
    {
        _renderThread->StopRender();
        needStartRender = true;
        _viewMatrix = view;
        _projMatrix = proj;
    }

    // has the frame been resized ?
    //
    const pxr::GfRect2i dataWindow = _GetDataWindow(renderPassState);
    if (_dataWindow != dataWindow) 
    {
        _renderThread->StopRender();
        needStartRender = true;
        _dataWindow = dataWindow;
        const pxr::GfVec3i dimensions(_dataWindow.GetWidth(), _dataWindow.GetHeight(), 1);
        _colorBuffer.Allocate(dimensions, pxr::HdFormatFloat16Vec4, false);

        _owner->ResizeBuffer(_dataWindow.GetWidth(), _dataWindow.GetHeight());
    }

    // empty AOVs ?
    //
    pxr::HdRenderPassAovBindingVector aovBindings = renderPassState->GetAovBindings();
    if (aovBindings.empty())
    {
        _renderThread->StopRender();
        needStartRender = true;
        pxr::HdRenderPassAovBinding colorAov;
        colorAov.aovName = HdAovTokens->color;
        colorAov.renderBuffer = &_colorBuffer;
        colorAov.clearValue = pxr::VtValue(pxr::GfVec4f(0.5f, 0.0f, 0.0f, 1.0f));
        aovBindings.push_back(colorAov);
    }

    //if (aovBindings.size() > 0 && aovBindings[0].aovName == HdAovTokens->color)
    //{
    //    std::cout << "found color" << std::endl;

    //    HdLighthouse2RenderBuffer* rb = static_cast<HdLighthouse2RenderBuffer*>(aovBindings[0].renderBuffer);
    //    //rb->Map();
    //    
    //    GfVec4f sample(1.0,0.0,0.0,1.0);
    //    float* sampleData = sample.data();
    //    for(int x =0;x<100;++x)
    //        for (int y = 0; y < 100; ++y)
    //            rb->Write(GfVec3i(x,y, 1), 4, sampleData);

    //    //rb->Unmap();
    //}

    if(_aovBindings != aovBindings)
    {
        _renderThread->StopRender();
        needStartRender = true;
        _aovBindings = aovBindings;
    }

    if( needStartRender )
    {
        _renderThread->StartRender();
    }

    auto* ltRenderer = _owner->GetRenderer();
    auto* ltRenderTarget = _owner->GetRenderTarget();
    auto* ltShader = _owner->GetShader();
    auto* ltCamera = ltRenderer->GetCamera();
    // get camera from renderPassState or from RenderSettings
    // renderPassState camera wins (from prman)
    pxr::SdfPath hdCameraPath;
    if (const HdLighthouse2Camera* const cam =
        static_cast<const HdLighthouse2Camera*>(
            renderPassState->GetCamera()))
    {
        hdCameraPath = cam->GetId();
    }
    else
    {
        // get from _owner->GetRenderSettings...

    }
    auto* hdCamera = static_cast<const HdLighthouse2Camera*>(
        GetRenderIndex()->GetSprim( HdPrimTypeTokens->camera, hdCameraPath));
    //auto* hdCamera = renderPassState->GetCamera();

    // update camera view
    // Do we need to get the sampleXform param here instead ?
    auto& passMatrix = hdCamera->GetTransform();

    mat4 passMatrixLT = mat4::Identity();
    // right
    passMatrixLT[0] = passMatrix.data()[0];
    passMatrixLT[4] = passMatrix.data()[1];
    passMatrixLT[8] = passMatrix.data()[2];
    // up
    passMatrixLT[1] = passMatrix.data()[4];
    passMatrixLT[5] = passMatrix.data()[5];
    passMatrixLT[9] = passMatrix.data()[6];
    // forward
    passMatrixLT[2] = passMatrix.data()[8] * (-1.0);
    passMatrixLT[6] = passMatrix.data()[9] * (-1.0);
    passMatrixLT[10] = passMatrix.data()[10] * (-1.0);
    // eye
    passMatrixLT[3] = passMatrix.data()[12];
    passMatrixLT[7] = passMatrix.data()[13];
    passMatrixLT[11] = passMatrix.data()[14];

    ltCamera->SetMatrix(passMatrixLT);

    const float focusDistance = hdCamera->GetFocusDistance();
    const float fStop = hdCamera->GetFStop();
    const float focalLength = hdCamera->GetFocalLength();
    const float verticalAperture = hdCamera->GetVerticalAperture();
    
    if (focusDistance > 0.0f)
        ltCamera->focalDistance = focusDistance;
    else
        ltCamera->focalDistance = 5.0f;
 
    if (fStop > 0.0f && focusDistance > 0.0f)
        ltCamera->aperture = fStop;
    else
        ltCamera->aperture = EPSILON; // small, for non - defocus ?

    if (focalLength > 0) 
    {
        const float r = verticalAperture / focalLength;
        const float fov = 2.0f * GfRadiansToDegrees(std::atan(0.5f * r));
        ltCamera->FOV = fov;
    }
    else
        ltCamera->FOV = 90.0f;

    ltCamera->pixelCount = make_int2(1, 1);

    // update render
    //
    bool needsRestart = _owner->UpdateScene();

    ltRenderer->SynchronizeSceneData();
    ltRenderer->Render( (ltCamera->Changed() || needsRestart) ? lighthouse2::Convergence::Restart : lighthouse2::Convergence::Converge);

    ltRenderer->WaitForRender();

    // draw render-target on screen
    //
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);

    ltShader->Bind();
    ltShader->SetInputTexture(0, "color", ltRenderTarget);
    ltShader->SetInputMatrix("view", mat4::Identity());
    ltShader->SetFloat("contrast", ltCamera->contrast);
    ltShader->SetFloat("brightness", ltCamera->brightness);
    ltShader->SetFloat("gamma", ltCamera->gamma);
    ltShader->SetInt("method", ltCamera->tonemapper);
    
    static GLuint vao = 0;
    if (!vao)
    {
        // generate buffers
        static const GLfloat verts[] = { -1, -1, 0,  1, -1, 0,  -1, 1, 0,  1, -1, 0,  -1, 1, 0,  1, 1, 0 };
        static const GLfloat uvdata[] = { 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0 }; // { 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0 };
        GLuint vertexBuffer = CreateVBO(verts, sizeof(verts));
        GLuint UVBuffer = CreateVBO(uvdata, sizeof(uvdata));
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        BindVBO(0, 3, vertexBuffer);
        BindVBO(1, 2, UVBuffer);
        glBindVertexArray(0);
        CheckGL();
    }
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    ltShader->Unbind();
}
