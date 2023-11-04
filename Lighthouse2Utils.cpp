#include "Lighthouse2Utils.h"

namespace Lighthouse2Utils
{
	void XformComponentsPxrToLighthouse2(
		const pxr::GfVec3f& t, 
		const pxr::GfMatrix3f& rm, 
		const pxr::GfVec3f& s, 
		mat4& o_matrix)
	{
		mat4 translation_m = mat4::Identity();
		translation_m(0, 3) = -t.data()[0];
		translation_m(1, 3) = -t.data()[1];
		translation_m(2, 3) = -t.data()[2];
		translation_m(3, 3) = 1;

		mat4 rotation_m = mat4::Identity();
		rotation_m(0, 0) = rm.GetRow(0)[0];
		rotation_m(1, 0) = rm.GetRow(0)[1];
		rotation_m(2, 0) = rm.GetRow(0)[2];
		rotation_m(3, 0) = 0;
		rotation_m(0, 1) = rm.GetRow(1)[0];
		rotation_m(1, 1) = rm.GetRow(1)[1];
		rotation_m(2, 1) = rm.GetRow(1)[2];
		rotation_m(3, 1) = 0;
		rotation_m(0, 2) = rm.GetRow(2)[0];
		rotation_m(1, 2) = rm.GetRow(2)[1];
		rotation_m(2, 2) = rm.GetRow(2)[2];
		rotation_m(3, 2) = 0;

		mat4 scale_m = mat4::Identity();
		scale_m(0, 0) = s.data()[0];
		scale_m(1, 1) = s.data()[1];
		scale_m(2, 2) = s.data()[2];
		scale_m(3, 3) = 1;

		o_matrix = translation_m * rotation_m;
	}

	void UpdateVertices(
		HostMesh* i_mesh,
		const std::vector<int>& tmpIndices,
		const std::vector<float3>& tmpVertices )
	{
		for (size_t i = 0; i < i_mesh->triangles.size(); i++)
		{
			HostTri& tri = i_mesh->triangles[i];
			const uint v0idx = tmpIndices[i * 3 + 0];
			const uint v1idx = tmpIndices[i * 3 + 1];
			const uint v2idx = tmpIndices[i * 3 + 2];
			tri.vertex0 = tmpVertices[v0idx];
			tri.vertex1 = tmpVertices[v1idx];
			tri.vertex2 = tmpVertices[v2idx];
		}
	}

	void BuildFromIndexedData(
		HostMesh* i_mesh,
		const std::vector<int>& tmpIndices,
		const std::vector<float3>& tmpVertices, // vertex
		const std::vector<float3>& tmpNormals, // vertex
		const std::vector<float2>& tmpUvs, //facevarying
		const int materialIdx)
	{
		// calculate values for consistent normal interpolation
		std::vector<float> tmpAlphas;
		tmpAlphas.resize(tmpVertices.size(), 1.0f); // we will have one alpha value per unique vertex
		for (size_t s = tmpIndices.size(), i = 0; i < s; i += 3)
		{
			const uint v0idx = tmpIndices[i + 0], v1idx = tmpIndices[i + 1], v2idx = tmpIndices[i + 2];
			const float3 vert0 = tmpVertices[v0idx], vert1 = tmpVertices[v1idx], vert2 = tmpVertices[v2idx];
			float3 N = normalize(cross(vert1 - vert0, vert2 - vert0));
			float3 vN0, vN1, vN2;
			if (tmpNormals.size() > 0)
			{
				vN0 = tmpNormals[v0idx], vN1 = tmpNormals[v1idx], vN2 = tmpNormals[v2idx];
				if (dot(N, vN0) < 0 && dot(N, vN1) < 0 && dot(N, vN2) < 0) N *= -1.0f; // flip if not consistent with vertex normals
			}
			else
			{
				// no normals supplied; copy face normal
				vN0 = vN1 = vN2 = N;
			}
			// Note: we clamp at approx. 45 degree angles; beyond this the approach fails.
			tmpAlphas[v0idx] = min(tmpAlphas[v0idx], dot(vN0, N));
			tmpAlphas[v1idx] = min(tmpAlphas[v1idx], dot(vN1, N));
			tmpAlphas[v2idx] = min(tmpAlphas[v2idx], dot(vN2, N));
		}
		for (size_t s = tmpAlphas.size(), i = 0; i < s; i++)
		{
			const float nnv = tmpAlphas[i]; // temporarily stored there
			tmpAlphas[i] = acosf(nnv) * (1 + 0.03632f * (1 - nnv) * (1 - nnv));
		}
		// build final mesh structures
		const size_t newTriangleCount = tmpIndices.size() / 3;
		size_t triIdx = i_mesh->triangles.size();
		i_mesh->triangles.resize(triIdx + newTriangleCount);
		for (size_t i = 0; i < newTriangleCount; i++, triIdx++)
		{
			HostTri& tri = i_mesh->triangles[triIdx];
			tri.material = materialIdx;
			const uint v0idx = tmpIndices[i * 3 + 0];
			const uint v1idx = tmpIndices[i * 3 + 1];
			const uint v2idx = tmpIndices[i * 3 + 2];
			const float3 v0pos = tmpVertices[v0idx];
			const float3 v1pos = tmpVertices[v1idx];
			const float3 v2pos = tmpVertices[v2idx];
			i_mesh->vertices.push_back(make_float4(v0pos, 1));
			i_mesh->vertices.push_back(make_float4(v1pos, 1));
			i_mesh->vertices.push_back(make_float4(v2pos, 1));
			const float3 N = normalize(cross(v1pos - v0pos, v2pos - v0pos));
			tri.Nx = N.x, tri.Ny = N.y, tri.Nz = N.z;
			tri.vertex0 = tmpVertices[v0idx];
			tri.vertex1 = tmpVertices[v1idx];
			tri.vertex2 = tmpVertices[v2idx];
			tri.alpha = make_float3(tmpAlphas[v0idx], tmpAlphas[v1idx], tmpAlphas[v2idx]);
			if (tmpNormals.size() > 0)
				tri.vN0 = tmpNormals[v0idx],
				tri.vN1 = tmpNormals[v1idx],
				tri.vN2 = tmpNormals[v2idx];
			else
				tri.vN0 = tri.vN1 = tri.vN2 = N;
			if (tmpUvs.size() > 0)
			{
				tri.u0 = tmpUvs[i * 3 + 0].x, tri.v0 = tmpUvs[i * 3 + 0].y;
				tri.u1 = tmpUvs[i * 3 + 1].x, tri.v1 = tmpUvs[i * 3 + 1].y;
				tri.u2 = tmpUvs[i * 3 + 2].x, tri.v2 = tmpUvs[i * 3 + 2].y;
				if (tri.u0 == tri.u1 && tri.u1 == tri.u2 && tri.v0 == tri.v1 && tri.v1 == tri.v2)
				{
					// this triangle uses only a single point on the texture; replace by single color material.
					int textureID = HostScene::materials[materialIdx]->color.textureID;
					if (textureID != -1)
					{
						HostTexture* texture = HostScene::textures[textureID];
						uint u = (uint)(tri.u0 * texture->width) % texture->width;
						uint v = (uint)(tri.v0 * texture->height) % texture->height;
						uint texel = ((uint*)texture->idata)[u + v * texture->width] & 0xffffff;
						tri.material = HostScene::FindOrCreateMaterialCopy(materialIdx, texel);
						int w = 0;
					}
				}
				// calculate tangent vector based on uvs
				float2 uv01 = make_float2(tri.u1 - tri.u0, tri.v1 - tri.v0);
				float2 uv02 = make_float2(tri.u2 - tri.u0, tri.v2 - tri.v0);
				if (dot(uv01, uv01) == 0 || dot(uv02, uv02) == 0)
				{
#if 1
					// PBRT:
					// https://github.com/mmp/pbrt-v3/blob/3f94503ae1777cd6d67a7788e06d67224a525ff4/src/shapes/triangle.cpp#L381
					if (std::abs(N.x) > std::abs(N.y))
						tri.T = make_float3(-N.z, 0, N.x) / std::sqrt(N.x * N.x + N.z * N.z);
					else
						tri.T = make_float3(0, N.z, -N.y) / std::sqrt(N.y * N.y + N.z * N.z);
#else
					tri.T = normalize(tri.vertex1 - tri.vertex0);
#endif
					tri.B = normalize(cross(N, tri.T));
				}
				else
				{
					tri.T = normalize((tri.vertex1 - tri.vertex0) * uv02.y - (tri.vertex2 - tri.vertex0) * uv01.y);
					tri.B = normalize((tri.vertex2 - tri.vertex0) * uv01.x - (tri.vertex1 - tri.vertex0) * uv02.x);
				}
				// catch bad tangents
				if (isnan(tri.T.x + tri.T.y + tri.T.z + tri.B.x + tri.B.y + tri.B.z))
				{
					tri.T = normalize(tri.vertex1 - tri.vertex0);
					tri.B = normalize(cross(N, tri.T));
				}
			}
			else
			{
				// no uv information; use edges to calculate tangent vectors
				tri.T = normalize(tri.vertex1 - tri.vertex0);
				tri.B = normalize(cross(N, tri.T));
			}
		}
	}

}
