cmake_minimum_required(VERSION 3.23)

project( Lighthouse2 )
set( DELEGATE_NAME hdLighthouse2 )

find_package( OpenGL REQUIRED )

set( HOUDINI_MAJ_MIN "19.5" )
set( HOUDINI_PATCH "773" )
set( HOUDINI_VERSION "Houdini ${HOUDINI_MAJ_MIN}.${HOUDINI_PATCH}" )
set( HOUDINI_ROOT "C:\\Program Files\\Side Effects Software\\${HOUDINI_VERSION}" )
set( HOUDINI_ROOT_USER "C:\\Users\\$ENV{USERNAME}\\Documents\\houdini${HOUDINI_MAJ_MIN}" )

set(LIGHTHOUSE2_ROOT "C:/Users/paolo/Desktop/code/lighthouse2")

set(LIGHTHOUSE2_INCLUDE_DIRECTORY 
    ${LIGHTHOUSE2_ROOT}/lib/RenderCore
    ${LIGHTHOUSE2_ROOT}/lib/zlib
    ${LIGHTHOUSE2_ROOT}/lib/glfw/include
    ${LIGHTHOUSE2_ROOT}/lib/glad/include
    ${LIGHTHOUSE2_ROOT}/lib/half2.2.0
    ${LIGHTHOUSE2_ROOT}/lib/RenderSystem
    ${LIGHTHOUSE2_ROOT}/lib/platform
    ${LIGHTHOUSE2_ROOT}/lib/freeimage/inc
    ${LIGHTHOUSE2_ROOT}/lib/taskflow 
)

set(LIGHTHOUSE2_LIB_DIRECTORY 
    ${LIGHTHOUSE2_ROOT}/lib/AntTweakBar/lib
    ${LIGHTHOUSE2_ROOT}/lib/zlib
    ${LIGHTHOUSE2_ROOT}/lib/RenderSystem/lib/Release
    ${LIGHTHOUSE2_ROOT}/lib/platform/lib/Release
)

set(LIGHTHOUSE2_LIBS 
    rendersystem
    platform
    libz-static
)

include(cmake/FindUsd.cmake)

add_library( ${DELEGATE_NAME} SHARED

    HdLighthouse2RendererPlugin.cpp
    HdLighthouse2RendererPlugin.h
    HdLighthouse2RenderDelegate.cpp
    HdLighthouse2RenderDelegate.h
    HdLighthouse2RenderPass.cpp
    HdLighthouse2RenderPass.h
    HdLighthouse2RenderBuffer.cpp
    HdLighthouse2RenderBuffer.h
    HdLighthouse2Mesh.cpp
    HdLighthouse2Mesh.h
    HdLighthouse2Material.cpp
    HdLighthouse2Material.h
    HdLighthouse2DomeLight.cpp
    HdLighthouse2DomeLight.h
    HdLighthouse2AreaLight.cpp
    HdLighthouse2AreaLight.h
    HdLighthouse2Camera.cpp
    HdLighthouse2Camera.h
    HdLighthouse2Instancer.cpp
    HdLighthouse2Instancer.h
    Lighthouse2Utils.cpp
    Lighthouse2Utils.h
)

target_link_libraries(
        ${DELEGATE_NAME}
        PUBLIC
        Usd::Usd
        OpenGL::GL
        ${LIGHTHOUSE2_LIBS}
        )

target_include_directories(
    ${DELEGATE_NAME}
    SYSTEM
    PRIVATE
        ${LIGHTHOUSE2_INCLUDE_DIRECTORY}
)

target_link_directories(
    ${DELEGATE_NAME}
    PRIVATE
        ${LIGHTHOUSE2_LIB_DIRECTORY}
)

target_compile_options(
        ${DELEGATE_NAME}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W3 /O2 /wd4273 /Zi /experimental:external /external:W0>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/Ob0 /Od>
        )

target_link_options(
        ${DELEGATE_NAME}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/ignore:4217 /ignore:4049> 
        )

configure_file(plugInfo.json plugInfo.json @ONLY)
configure_file(UsdRenderers.json UsdRenderers.json @ONLY)
configure_file(${DELEGATE_NAME}.json ${DELEGATE_NAME}.json @ONLY)

install(TARGETS ${DELEGATE_NAME} DESTINATION ${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/dso/usd_plugins)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${DELEGATE_NAME}.json DESTINATION ${HOUDINI_ROOT_USER}/packages)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/UsdRenderers.json DESTINATION ${HOUDINI_ROOT_USER}/${DELEGATE_NAME})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json DESTINATION ${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/dso/usd_plugins)

set(files_to_copy
    ${LIGHTHOUSE2_ROOT}/apps/viewerapp/freeimage.dll,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}
    ${LIGHTHOUSE2_ROOT}/apps/viewerapp/freetype.dll,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}
    ${LIGHTHOUSE2_ROOT}/coredlls/Release/RenderCore_Optix7.dll,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/cores
    ${LIGHTHOUSE2_ROOT}/lib/rendercore_optix7/core_api.cpp,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/rendercore_optix7
    ${LIGHTHOUSE2_ROOT}/lib/rendercore_optix7/core_mesh.cpp,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/rendercore_optix7
    ${LIGHTHOUSE2_ROOT}/lib/rendercore_optix7/core_mesh.h,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/rendercore_optix7
    ${LIGHTHOUSE2_ROOT}/lib/rendercore_optix7/core_settings.h,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/rendercore_optix7
    ${LIGHTHOUSE2_ROOT}/lib/rendercore_optix7/rendercore.cpp,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/rendercore_optix7
    ${LIGHTHOUSE2_ROOT}/lib/rendercore_optix7/rendercore.h,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/rendercore_optix7
    ${LIGHTHOUSE2_ROOT}/lib/RenderSystem/common_settings.h,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/RenderSystem
    ${LIGHTHOUSE2_ROOT}/lib/RenderSystem/common_functions.h,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/RenderSystem
    ${LIGHTHOUSE2_ROOT}/lib/RenderSystem/common_bluenoise.h,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/RenderSystem
    ${LIGHTHOUSE2_ROOT}/lib/RenderSystem/common_classes.h,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/RenderSystem
    ${LIGHTHOUSE2_ROOT}/lib/RenderSystem/common_types.h,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/RenderSystem
    ${CMAKE_SOURCE_DIR}/ibls/qwantani_puresky_2k.hdr,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/ibls
)

file( GLOB_RECURSE GLOB_RESULT ${LIGHTHOUSE2_ROOT}/lib/CUDA/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/CUDA)
endforeach()

file( GLOB_RECURSE GLOB_RESULT ${LIGHTHOUSE2_ROOT}/lib/OptiX/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/OptiX)
endforeach()

file( GLOB_RECURSE GLOB_RESULT ${LIGHTHOUSE2_ROOT}/lib/OptiX7/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/OptiX7)
endforeach()

file( GLOB_RECURSE GLOB_RESULT ${LIGHTHOUSE2_ROOT}/apps/viewerapp/data/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/data)
endforeach()

file( GLOB_RECURSE GLOB_RESULT ${LIGHTHOUSE2_ROOT}/apps/viewerapp/shaders/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/shaders)
endforeach()

file( GLOB_RECURSE GLOB_RESULT ${LIGHTHOUSE2_ROOT}/lib/rendercore_optix7/kernels/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/rendercore_optix7/kernels)
endforeach()

file( GLOB_RECURSE GLOB_RESULT ${LIGHTHOUSE2_ROOT}/lib/rendercore_optix7/optix/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/rendercore_optix7/optix)
endforeach()

file( GLOB_RECURSE GLOB_RESULT ${LIGHTHOUSE2_ROOT}/lib/RenderSystem/materials/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/lib/RenderSystem/materials)
endforeach()

foreach( f IN LISTS files_to_copy )
    string(REPLACE "," ";" t ${f})
    list(GET t 0 f_orig)
    list(GET t 1 f_dest)
    configure_file( ${f_orig} ${f_orig} @ONLY )
    install(FILES ${f_orig} DESTINATION ${f_dest})
endforeach()
