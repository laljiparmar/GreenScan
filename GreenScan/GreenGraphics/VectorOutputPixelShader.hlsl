#include "Header.hlsli"
Texture2D<int> DepthTexture  : register(t0);

float3 PosOf(float x, float y)
{
	int3 id = DepthCoords(float2(x, y));
	float depth = DepthTexture.Load(id);
	return mul(float4(DepthSize.x * x * depth, DepthSize.y * y * depth, depth, 1), DepthInvIntrinsics).xyz;
}

bool CalculateWorld(float2 uv, out float3 posx)
{
	posx = PosOf(uv.x, uv.y);
	float3 posu = PosOf(uv.x, uv.y + DepthStep.y);
	float3 posd = PosOf(uv.x, uv.y - DepthStep.y);
	float3 posl = PosOf(uv.x - DepthStep.x, uv.y);
	float3 posr = PosOf(uv.x + DepthStep.x, uv.y);
	return pow(length(posu - posd) / posx.z, 2) + pow(length(posr - posl) / posx.z, 2) > TriangleLimit;
}


float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float3 pos;
	if(CalculateWorld(v.Texture, pos))	
		return float4(pos, 1);
	else
		return 0;
}