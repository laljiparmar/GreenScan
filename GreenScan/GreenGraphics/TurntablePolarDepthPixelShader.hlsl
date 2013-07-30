#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float4 data = Texture.Sample(Sampler, v.Texture);
	if(data.y > 0.f || data.w > 0.f)
		return float4(data.x / data.y, data.z / data.w, 0.f, 1.f);
	else
		return 0.f;
}