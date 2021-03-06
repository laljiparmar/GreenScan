#include "Header.hlsli"
Texture2D<float> Texture  : register(t0);
Texture1D<float> Coeffs  : register(t1);
SamplerState Sampler : register(s0);

float main(VertexPositionTextureOut v) : SV_TARGET
{
	float2 pos, center = v.Texture;
	float depth = Texture.Sample(Sampler, center);
	if (depth <= MinDepth) return 0;

	float sum = 0;
	float gain = 0;
	float coeff;

	for (int i = 0; i < GaussCoeffCount; i++)
	{
		pos = center + float2((i - GaussCoeffCount / 2) * DepthStep.x, 0.f);
		depth = Texture.Sample(Sampler, pos);
		if (depth > MinDepth)
		{
			coeff = Coeffs.Sample(Sampler, (float)i / (GaussCoeffCount - 1));
			sum += depth * coeff;
			gain += coeff;
		}
	}

	return sum / gain;
}