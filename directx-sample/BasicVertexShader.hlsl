#include "BasicShaderHeader.hlsli"

Vertex BasicVS( float4 pos : POSITION, float2 uv : TEXCOORD )
{
	Vertex output;
	output.svpos = mul(mat, pos);
	output.uv = uv;
	return output;
}