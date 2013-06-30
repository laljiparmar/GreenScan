#pragma once
#pragma unmanaged
#include "stdafx.h"
#include "Helper.h"
#include "GreenGraphicsVertexDefinitions.h"
using namespace DirectX;
using namespace Gdiplus;

namespace Green
{
	namespace Graphics
	{
		class VertexBufferBase
		{
		protected:
			VertexDefinition* Definition;
		public:
			VertexDefinition* GetDefinition()
			{
				return Definition;
			}
		};

		class VertexShader
		{
		private:
			void* Source;
			int SourceLength;
			ID3D11Device* Device;
			ID3D11DeviceContext* Context;
			ID3D11InputLayout* InputLayout;
		public:
			ID3D11VertexShader* Shader;			
			VertexShader(ID3D11Device* device, LPWSTR path)
			{
				Source = 0;
				SourceLength = 0;
				LoadFile(path, Source, SourceLength);
				InputLayout = 0;
				Error(device->CreateVertexShader(Source, SourceLength, 0, &Shader));
				Device = device;
				device->GetImmediateContext(&Context);
			}
			void SetInputLayout(VertexDefinition* vertexDefinition)
			{
				if(InputLayout != 0) InputLayout->Release();
				Error(Device->CreateInputLayout(
					vertexDefinition->Description, 
					vertexDefinition->ElementCount,
					Source, SourceLength, &InputLayout));
			}
			void Apply()
			{
				Context->VSSetShader(Shader, 0, 0);
				Context->IASetInputLayout(InputLayout);
			}
			~VertexShader()
			{
				Shader->Release();
				Context->Release();
				if(InputLayout != 0) InputLayout->Release();
				free(Source);
			}
		};

		template <class T>
		class VertexBuffer : public VertexBufferBase
		{
		private:
			ID3D11DeviceContext* Context;
			ID3D11Buffer* Buffer;
		public:			
			VertexBuffer(ID3D11Device* device, int size, VertexDefinition* definition, T* vertices = 0)
			{
				D3D11_BUFFER_DESC bd;
				ZeroMemory(&bd, sizeof(bd));
				bd.ByteWidth = sizeof(T) * size;
				bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				if(vertices == 0)
				{
					bd.Usage = D3D11_USAGE_DYNAMIC;
					bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					Error(device->CreateBuffer(&bd, 0, &Buffer));
				}
				else
				{
					bd.Usage = D3D11_USAGE_IMMUTABLE;
					bd.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = vertices;

					Error(device->CreateBuffer(&bd, &sd, &Buffer));
				}
				
				Definition = definition;
				device->GetImmediateContext(&Context);
			}

			void Load(T* vertices, int count)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Context->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				memcpy(ms.pData, vertices, sizeof(T) * count);
				Context->Unmap(Buffer, 0);
			}

			void Set()
			{
				UINT stride = sizeof(T);
				UINT offset = 0;
				Context->IASetVertexBuffers(0, 1, &Buffer, &stride, &offset);
			}

			~VertexBuffer()
			{
				Buffer->Release();
				Context->Release();
			}
		};

		template <class T>
		class IndexBuffer
		{
		private:
			ID3D11DeviceContext* Context;
			ID3D11Buffer* Buffer;
		public:			
			IndexBuffer(ID3D11Device* device, int size, T* indicies = 0)
			{
				D3D11_BUFFER_DESC bd;
				ZeroMemory(&bd, sizeof(bd));
				bd.ByteWidth = sizeof(T) * size;
				bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
				if(indicies == 0)
				{
					bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					bd.Usage = D3D11_USAGE_DYNAMIC;
					Error(device->CreateBuffer(&bd, 0, &Buffer));
				}
				else
				{
					bd.Usage = D3D11_USAGE_IMMUTABLE;
					bd.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = indicies;

					Error(device->CreateBuffer(&bd, &sd, &Buffer));
				}
				device->GetImmediateContext(&Context);
			}

			void Load(T* indicies, int count)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Context->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				memcpy(ms.pData, vertices, sizeof(T) * count);
				Context->Unmap(Buffer, 0);
			}

			void Set()
			{
				if(sizeof(T) == 4)
					Context->IASetIndexBuffer(Buffer, DXGI_FORMAT_R32_UINT, 0);
				else
					Context->IASetIndexBuffer(Buffer, DXGI_FORMAT_R16_UINT, 0);
			}

			~IndexBuffer()
			{
				Buffer->Release();
				Context->Release();
			}
		};

		template <class T>
		class ConstantBuffer
		{
		private:
			ID3D11DeviceContext* Context;
			ID3D11Buffer* Buffer;
		public:			
			ConstantBuffer(ID3D11Device* device)
			{
				D3D11_BUFFER_DESC bd;
				int size = sizeof(T);
				bd.ByteWidth = (size % 16 == 0 ? size : size + 16 - size % 16);
				bd.Usage = D3D11_USAGE_DEFAULT;				
				bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				bd.CPUAccessFlags = 0;
				bd.MiscFlags = 0;
				bd.StructureByteStride = 0;
				
				Error(device->CreateBuffer(&bd, 0, &Buffer));
				device->GetImmediateContext(&Context);
			}

			void Update(T* data)
			{
				Context->UpdateSubresource(Buffer, 0, 0, data, 0, 0);
			}

			void SetForVS(int slot = 0)
			{
				Context->VSSetConstantBuffers(slot, 1, &Buffer);
			}

			void SetForPS(int slot = 0)
			{
				Context->PSSetConstantBuffers(slot, 1, &Buffer);
			}

			void SetForCS(int slot = 0)
			{
				Context->CSSetConstantBuffers(slot, 1, &Buffer);
			}

			void SetForGS(int slot = 0)
			{
				Context->GSSetConstantBuffers(slot, 1, &Buffer);
			}

			~ConstantBuffer()
			{
				Buffer->Release();
				Context->Release();
			}
		};

		class GeometryShader
		{
		private:
			ID3D11GeometryShader* Shader;
			ID3D11DeviceContext* Context;
		public:			
			GeometryShader(ID3D11Device* device, LPWSTR path)
			{
				void* source = 0;
				int sourceLength = 0;
				LoadFile(path, source, sourceLength);
				Error(device->CreateGeometryShader(source, sourceLength, 0, &Shader));
				free(source);
				device->GetImmediateContext(&Context);
			}

			void Apply()
			{
				Context->GSSetShader(Shader, 0, 0);
			}

			~GeometryShader()
			{
				Shader->Release();
				Context->Release();
			}
		};

		class PixelShader
		{
		private:
			ID3D11PixelShader* Shader;
			ID3D11DeviceContext* Context;
		public:			
			PixelShader(ID3D11Device* device, LPWSTR path)
			{
				void* source = 0;
				int sourceLength = 0;
				LoadFile(path, source, sourceLength);
				Error(device->CreatePixelShader(source, sourceLength, 0, &Shader));
				free(source);
				device->GetImmediateContext(&Context);
			}

			void Apply()
			{
				Context->PSSetShader(Shader, 0, 0);
			}

			~PixelShader()
			{
				Shader->Release();
				Context->Release();
			}
		};

		class SamplerState
		{
		private:
			ID3D11SamplerState *State;
			ID3D11DeviceContext *Context;
		public:
			SamplerState(ID3D11Device* device, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
			{
				D3D11_SAMPLER_DESC sd;
				ZeroMemory(&sd, sizeof(sd));
				sd.Filter = filter;
				sd.MaxAnisotropy = 0;
				sd.AddressU = addressMode;
				sd.AddressV = addressMode;
				sd.AddressW = addressMode;
				sd.MipLODBias = 0.f;
				sd.MinLOD = 0.f;
				sd.MaxLOD = D3D11_FLOAT32_MAX;
				sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
				for(int i = 0; i < 4; i++)
					sd.BorderColor[i] = 0.f;
				Error(device->CreateSamplerState(&sd, &State));
				device->GetImmediateContext(&Context);
			}

			void SetForVS(int slot = 0)
			{
				Context->VSSetSamplers(slot, 1, &State);
			}

			void SetForPS(int slot = 0)
			{
				Context->PSSetSamplers(slot, 1, &State);
			}

			~SamplerState()
			{
				State->Release();
				Context->Release();
			}
		};

		class Texture1D
		{
		private:
			ID3D11Texture1D* Texture;
			ID3D11ShaderResourceView* ResourceView;
			ID3D11DeviceContext* Context;
			int Length;
		public:
			Texture1D(ID3D11Device* device, int length, DXGI_FORMAT format, void* data = 0, int stride = 0) : Length(length)
			{
				D3D11_TEXTURE1D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = length;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = format;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;				
				desc.MiscFlags = 0;
				if(data == 0)
				{
					desc.Usage = D3D11_USAGE_DYNAMIC;
					desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					Error(device->CreateTexture1D(&desc, 0, &Texture));
				}
				else
				{
					desc.Usage = D3D11_USAGE_IMMUTABLE;
					desc.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = data;
					sd.SysMemPitch = stride;
					sd.SysMemSlicePitch = 0;
					Error(device->CreateTexture1D(&desc, &sd, &Texture));
				}				

				D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc;
				ZeroMemory(&textureViewDesc, sizeof(textureViewDesc));
				textureViewDesc.Format = format; 
				textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D; 
				textureViewDesc.Texture2D.MipLevels = 1; 
				textureViewDesc.Texture2D.MostDetailedMip = 0; 

				Error(device->CreateShaderResourceView(Texture, &textureViewDesc, &ResourceView));

				device->GetImmediateContext(&Context);
			}

			static Texture1D* FromFile(ID3D11Device* device, LPWSTR path)
			{
				Bitmap bitmap(path);
				int width = bitmap.GetWidth();
				int height = bitmap.GetHeight();
				Gdiplus::Rect lockRect(0, 0, width, height);
				BitmapData bitmapData;
				bitmap.LockBits(&lockRect, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
				Texture1D* texture = new Texture1D(device, width, DXGI_FORMAT_B8G8R8A8_UNORM, bitmapData.Scan0, bitmapData.Stride);
				bitmap.UnlockBits(&bitmapData);
				return texture;
			}

			template <class T> void Load(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Context->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
				memcpy(ms.pData, data, Length * sizeof(T));
				Context->Unmap(Texture, 0);
			}

			void SetForVS(int slot = 0)
			{
				Context->VSSetShaderResources(slot, 1, &ResourceView);
			}

			void SetForPS(int slot = 0)
			{
				Context->PSSetShaderResources(slot, 1, &ResourceView);
			}

			~Texture1D()
			{
				Texture->Release();
				ResourceView->Release();
				Context->Release();
			}
		};

		class Texture2D
		{
		private:
			ID3D11Texture2D* Texture;
			ID3D11ShaderResourceView* ResourceView;
			ID3D11DeviceContext* Context;
			int Width, Height;
		public:
			Texture2D(ID3D11Device* device, int width, int height, DXGI_FORMAT format, void* data = 0, int stride = 0) : Width(width), Height(height)
			{
				D3D11_TEXTURE2D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = width;
				desc.Height = height;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = format;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;				
				desc.MiscFlags = 0;
				if(data == 0)
				{
					desc.Usage = D3D11_USAGE_DYNAMIC;
					desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					Error(device->CreateTexture2D(&desc, 0, &Texture));
				}
				else
				{
					desc.Usage = D3D11_USAGE_IMMUTABLE;
					desc.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = data;
					sd.SysMemPitch = stride;
					sd.SysMemSlicePitch = 0;
					Error(device->CreateTexture2D(&desc, &sd, &Texture));
				}				

				D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc;
				ZeroMemory(&textureViewDesc, sizeof(textureViewDesc));
				textureViewDesc.Format = format; 
				textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; 
				textureViewDesc.Texture2D.MipLevels = 1; 
				textureViewDesc.Texture2D.MostDetailedMip = 0; 

				Error(device->CreateShaderResourceView(Texture, &textureViewDesc, &ResourceView));

				device->GetImmediateContext(&Context);
			}

			static Texture2D* FromFile(ID3D11Device* device, LPWSTR path)
			{
				Bitmap bitmap(path);
				int width = bitmap.GetWidth();
				int height = bitmap.GetHeight();
				Gdiplus::Rect lockRect(0, 0, width, height);
				BitmapData bitmapData;
				bitmap.LockBits(&lockRect, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
				Texture2D* texture = new Texture2D(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, bitmapData.Scan0, bitmapData.Stride);
				bitmap.UnlockBits(&bitmapData);
				return texture;
			}

			template <class T> void Load(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Context->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
				memcpy(ms.pData, data, Width * Height * sizeof(T));
				Context->Unmap(Texture, 0);
			}

			void Load24bit(void* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Context->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
				int size = Width * Height;
				byte* pTarget = (byte*)ms.pData;
				byte* pSource = (byte*)data;
				for(int i = 0; i < size; i++)
				{
					*pTarget++ = *pSource++;
					*pTarget++ = *pSource++;
					*pTarget++ = *pSource++;
					*pTarget++ = 255;
				}
				Context->Unmap(Texture, 0);
			}

			void SetForVS(int slot = 0)
			{
				Context->VSSetShaderResources(slot, 1, &ResourceView);
			}

			void SetForPS(int slot = 0)
			{
				Context->PSSetShaderResources(slot, 1, &ResourceView);
			}

			~Texture2D()
			{
				Texture->Release();
				ResourceView->Release();
				Context->Release();
			}
		};

		class DepthBuffer
		{
		private:
			ID3D11Texture2D* Texture;
			ID3D11DepthStencilView* DepthStencilView;
			ID3D11DeviceContext* DeviceContext;
			ID3D11RenderTargetView* RenderTargetView;
		public:
			DepthBuffer(ID3D11RenderTargetView* target)
			{
				target->AddRef();
				ID3D11Device* device;
				target->GetDevice(&device);
				RenderTargetView = target;

				ID3D11Resource* resource;
				target->GetResource(&resource);
				ID3D11Texture2D* tex2D = (ID3D11Texture2D*)resource;
				D3D11_TEXTURE2D_DESC tex2Dd;
				tex2D->GetDesc(&tex2Dd);
				resource->Release();

				D3D11_TEXTURE2D_DESC td;
				ZeroMemory(&td, sizeof(td));
				td.Width = tex2Dd.Width;
				td.Height = tex2Dd.Height;
				td.MipLevels = 1;
				td.ArraySize = 1;
				td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				td.SampleDesc.Count = 1;
				td.SampleDesc.Quality = 0;
				td.Usage = D3D11_USAGE_DEFAULT;
				td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
				td.CPUAccessFlags = 0;
				td.MiscFlags = 0;
				
				Error(device->CreateTexture2D(&td, 0, &Texture));

				D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
				ZeroMemory(&dsvd, sizeof(dsvd));
				dsvd.Format = td.Format;
				dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvd.Texture2D.MipSlice = 0;

				Error(device->CreateDepthStencilView(Texture, &dsvd, &DepthStencilView));
				
				device->GetImmediateContext(&DeviceContext);	
				device->Release();
			}

			void Set()
			{
				DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
			}

			void Clear()
			{
				DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
			}

			~DepthBuffer()
			{
				DeviceContext->Release();
				DepthStencilView->Release();
				Texture->Release();
				RenderTargetView->Release();
			}
		};

		class Quad
		{
		private:
			ID3D11DeviceContext* Context;
			VertexBuffer<VertexPositionTexture>* VB;
		public:
			Quad(ID3D11Device* device)
			{	
				VertexPositionTexture* vertices = new VertexPositionTexture[4];
				vertices[0].Position = XMFLOAT3(-1.f, -1.f, 0.f);
				vertices[0].Texture = XMFLOAT2(1.f, 1.f);
				vertices[1].Position = XMFLOAT3(-1.f, 1.f, 0.f);
				vertices[1].Texture = XMFLOAT2(1.f, 0.f);
				vertices[2].Position = XMFLOAT3(1.f, -1.f, 0.f);
				vertices[2].Texture = XMFLOAT2(0.f, 1.f);
				vertices[3].Position = XMFLOAT3(1.f, 1.f, 0.f);
				vertices[3].Texture = XMFLOAT2(0.f, 0.f);
				VB = new VertexBuffer<VertexPositionTexture>(device, 4, VertexDefinition::VertexPositionTexture, vertices);
				delete [4] vertices;
				device->GetImmediateContext(&Context);
			}

			~Quad()
			{
				delete VB;
				Context->Release();
			}

			VertexDefinition* GetVertexDefinition()
			{
				return VB->GetDefinition();
			}

			void Draw()
			{
				VB->Set();
				Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				Context->Draw(4, 0);
			}
		};

		class Plane
		{
		private:
			VertexBuffer<VertexPositionTexture>* VB;
			IndexBuffer<unsigned int>* IB;
			ID3D11DeviceContext* Context;
			int IndexCount;
		public:
			Plane(ID3D11Device* device, int width, int height)
			{
				float xstep = 2.f / (width - 1), xtexstep = 1.f / (width-1), xstart = -1.f;
				float ystep = -2.f / (height - 1) , ytexstep = 1.f / (height-1), ystart = 1.f;

				int vertexCount = width * height;
				VertexPositionTexture* vertices = new VertexPositionTexture[vertexCount];
				for (int j = 0; j < height; j++)
				{
					for (int i = 0; i < width; i++)
					{
						vertices[j * width + i] = VertexPositionTexture(
							XMFLOAT3(xstart + i * xstep, ystart + j * ystep, 0.f),
							XMFLOAT2(i * xtexstep, j * ytexstep));
					}
				}
				VB = new VertexBuffer<VertexPositionTexture>(device, width * height, VertexDefinition::VertexPositionTexture, vertices);
				delete [vertexCount] vertices;
				

				unsigned int triangleWidth = width - 1, triangleHeight = height - 1;
				IndexCount = triangleWidth * triangleHeight * 6;
				unsigned int* indicies = new unsigned int[IndexCount];
				unsigned int triangleIndex;
				for (unsigned int j = 0; j < triangleHeight; j++)
				{
					for (unsigned int i = 0; i < triangleWidth; i++)
					{
						triangleIndex = (j * triangleWidth + i) * 6;
						indicies[triangleIndex] = j * width + i;
						indicies[triangleIndex + 1] = j * width + i + 1;
						indicies[triangleIndex + 2] = (j + 1) * width + i;
						indicies[triangleIndex + 3] = j * width + i + 1;
						indicies[triangleIndex + 4] = (j + 1) * width + i + 1;
						indicies[triangleIndex + 5] = (j + 1) * width + i;
					}
				}
				IB = new IndexBuffer<unsigned int>(device, IndexCount, indicies);
				delete [IndexCount] indicies;

				device->GetImmediateContext(&Context);
			}

			VertexDefinition* GetVertexDefinition()
			{
				return VB->GetDefinition();
			}

			void Draw()
			{
				VB->Set();
				IB->Set();
				Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Context->DrawIndexed(IndexCount, 0, 0);
			}

			~Plane()
			{
				delete VB;
				delete IB;
				Context->Release();
			}
		};


	}
}