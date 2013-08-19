#define Pi 3.14159f
#define MinDepth 0.4f
#define MaxDepth 10.f
#define GaussCoeffCount 9

struct VertexPositionTextureIn
{
	float3 Position : POSITION0;
	float2 Texture : TEXCOORD0;
};

struct VertexPositionTextureOut
{
	float4 Position : SV_POSITION;
	float2 Texture : TEXCOORD0;
};

struct VertexPositionColorIn
{
	float3 Position : POSITION0;
	float4 Color : COLOR0;
};

struct VertexPositionColorOut
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR0;
};

struct VertexPositionTextureDepth
{
    float4 Position : SV_POSITION;
	float3 WorldPosition : POSITION1;
	float3 Normal: NORMAL0;
	float Depth : NORMAL1;
	float2 Texture : TEXCOORD0;
};

struct VertexPolar
{
    float4 Position : SV_POSITION;
	float3 World : POSITION1;
	float2 Texture : TEXCOORD0;
};

cbuffer CommonConstants : register(b0)
{
	float4x4 SceneRotation;
	float2 AspectScale;
	float2 Move;
	float Scale;
};

cbuffer DepthAndColorConstants : register(b1)
{
	float4x4 DepthInvIntrinsics;
	float4x4 ReprojectionTransform;
	float4x4 WorldTransform;
	float4x4 NormalTransform;
	float4x4 DepthToColorTransform;
	float4x4 WorldToColorTransform;
	float4x4 SaveTransform;
	float2 DepthStep;
	float2 DepthSaveStep;
	float2 ColorMove;
	float2 ColorScale;
	int2 DepthSize;
	int2 ColorSize;
	int2 SaveSize;
	float DepthMaximum;
	float DepthMinimum;
	float ShadingPeriode;
	float ShadingPhase;
	float TriangleLimit;
};

cbuffer TurntableConstants : register(b1)
{
	float4x4 TurntableToScreenTransform;
	float4x4 DepthToTurntableTransform;
	float4x4 DepthToTextureTransform;
	float4x4 ModelToScreenTransform;
	float2 CorePosition;
	float2 ClipLimit;
	float2 TextureMove;
	float2 TextureScale;
	int2 DepthResolution;
	int2 ColorResolution;
	int2 ModelResolution;
	float Side;
};

float ToDepth(float raw)
{
	float depth = raw / 8000.f;
	if(depth < MaxDepth)
		return depth;
	else
		return 0.f;
}

int3 DepthCoords(float2 uv)
{
	return int3((int)(uv.x * DepthSize.x), (int)(uv.y * DepthSize.y), 0);
}

int3 SaveCoords(float2 uv)
{
	return int3((int)(uv.x * SaveSize.x), (int)(uv.y * SaveSize.y), 0);
}