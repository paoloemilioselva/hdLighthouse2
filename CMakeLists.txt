cmake_minimum_required(VERSION 3.23)

project( Lighthouse2 )
set( DELEGATE_NAME hdLighthouse2 )

find_package( OpenGL REQUIRED )

if(NOT TARGET_CONFIG)
    set( TARGET_CONFIG "houdini-19.5-773-39")
    #set( TARGET_CONFIG "houdini-20.0-506-310")
    #set( TARGET_CONFIG "usd-23.08")
endif()
string(REPLACE "-" ";" _t_config ${TARGET_CONFIG})
list(LENGTH _t_config _t_config_length)
if(_t_config_length LESS 2)
    MESSAGE(FATAL_ERROR "Wrong TARGET_CONFIG value " ${TARGET_CONFIG} )
endif()
list(GET _t_config 0 _target_name )
if( _target_name MATCHES "houdini" )
    list(GET _t_config 1 HOUDINI_MAJ_MIN )
    list(GET _t_config 2 HOUDINI_PATCH )
    list(GET _t_config 3 HOUDINI_PYTHON_VERSION )
    set( HOUDINI_VERSION "Houdini ${HOUDINI_MAJ_MIN}.${HOUDINI_PATCH}" )
    set( HOUDINI_ROOT "C:\\Program Files\\Side Effects Software\\${HOUDINI_VERSION}" )
    set( HOUDINI_ROOT_USER "C:\\Users\\$ENV{USERNAME}\\Documents\\houdini${HOUDINI_MAJ_MIN}" )
    MESSAGE(STATUS "Building for Houdini" )
    MESSAGE(STATUS "HOUDINI_VERSION = " ${HOUDINI_VERSION} )
    MESSAGE(STATUS "HOUDINI_ROOT = " ${HOUDINI_ROOT} )
    MESSAGE(STATUS "HOUDINI_ROOT_USER = " ${HOUDINI_ROOT_USER} )
    MESSAGE(STATUS "HOUDINI_PYTHON_VERSION = " ${HOUDINI_PYTHON_VERSION} )
elseif( _target_name MATCHES "usd" )
    MESSAGE(STATUS "Building for OpenUSD" )
else()
    MESSAGE(FATAL_ERROR "Invalid target " ${_target_name})
endif()

include(cmake/FindUsd.cmake)


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
)

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

target_link_libraries( ${DELEGATE_NAME}
        PUBLIC
        Usd::Usd
        OpenGL::GL
        ${LIGHTHOUSE2_LIBS} )

target_include_directories( ${DELEGATE_NAME} SYSTEM PRIVATE ${LIGHTHOUSE2_INCLUDE_DIRECTORY} )

target_link_directories( ${DELEGATE_NAME} PRIVATE ${LIGHTHOUSE2_LIB_DIRECTORY} )

target_compile_options( ${DELEGATE_NAME}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W3 /O2 /wd4273 /Zi /experimental:external /external:W0>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/Ob0 /Od> )

target_link_options( ${DELEGATE_NAME}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/ignore:4217 /ignore:4049> )

add_custom_command(
    TARGET ${DELEGATE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/${DELEGATE_NAME}.dll
        ${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/dso/usd_plugins/${DELEGATE_NAME}.dll )

set(files_to_copy
    ${CMAKE_SOURCE_DIR}/${DELEGATE_NAME}.json,${HOUDINI_ROOT_USER}/packages
    ${CMAKE_SOURCE_DIR}/UsdRenderers.json,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}
    ${CMAKE_SOURCE_DIR}/plugInfo.json,${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/dso/usd_plugins
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

set( folders_to_copy 
    lib/CUDA
    lib/CUDA/shared_host_code
    lib/CUDA/shared_kernel_code
    lib/OptiX
    lib/OptiX/include
    lib/OptiX/include/internal
    lib/OptiX/include/optix_prime
    lib/OptiX/include/optixu
    lib/OptiX/lib64
    lib/OptiX7
    lib/OptiX7/include
    lib/OptiX7/include/internal
    lib/rendercore_optix7/kernels
    lib/rendercore_optix7/optix
    lib/RenderSystem/materials/pbrt

)

foreach( d IN LISTS folders_to_copy )
    file( GLOB GLOB_RESULT ${LIGHTHOUSE2_ROOT}/${d}/* )
    foreach( f IN LISTS GLOB_RESULT )
        list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/Lighthouse2/${d})
    endforeach()
endforeach()

file( GLOB GLOB_RESULT ${LIGHTHOUSE2_ROOT}/apps/viewerapp/data/noise/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/data/noise)
endforeach()

file( GLOB GLOB_RESULT ${LIGHTHOUSE2_ROOT}/apps/viewerapp/shaders/* )
foreach( f IN LISTS GLOB_RESULT )
    list(APPEND files_to_copy ${f},${HOUDINI_ROOT_USER}/${DELEGATE_NAME}/shaders)
endforeach()

message(STATUS "Copying files in POST_BUILD:")
foreach( f IN LISTS files_to_copy )
    string(REPLACE "," ";" t ${f})
    list(GET t 0 f_orig)
    list(GET t 1 f_dest)
    file(TO_CMAKE_PATH "${f_orig}" f_orig)
    file(TO_CMAKE_PATH "${f_dest}" f_dest)
    #message(STATUS " - ${f_orig} -> ${f_dest}")
    add_custom_command( TARGET ${DELEGATE_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${f_dest} )
    add_custom_command( TARGET ${DELEGATE_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${f_orig} ${f_dest} )
    #configure_file( ${f_orig} ${f_orig} @ONLY )
    #install(FILES ${f_orig} DESTINATION ${f_dest})
endforeach()
