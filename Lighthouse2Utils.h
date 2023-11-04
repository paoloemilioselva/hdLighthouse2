#ifndef LIGHTHOUSE2UTILS_H
#define LIGHTHOUSE2UTILS_H

#include "platform.h"
#include "rendersystem.h"
#include "common_types.h"
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/matrix3f.h>

namespace Lighthouse2Utils
{
	void XformComponentsPxrToLighthouse2(
		const pxr::GfVec3f& t, 
		const pxr::GfMatrix3f& rm,
		const pxr::GfVec3f& s,
		mat4& o_matrix);

	void UpdateVertices(
		HostMesh* i_mesh,
		const std::vector<int>& tmpIndices,
		const std::vector<float3>& tmpVertices);

	void BuildFromIndexedData(
		HostMesh* i_mesh,
		const std::vector<int>& tmpIndices,
		const std::vector<float3>& tmpVertices, // vertex
		const std::vector<float3>& tmpNormals, // vertex
		const std::vector<float2>& tmpUvs, //facevarying
		const int materialIdx);

}

#endif
