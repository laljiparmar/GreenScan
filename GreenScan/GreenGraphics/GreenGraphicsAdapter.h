#pragma once
#pragma unmanaged
#include "GreenGraphics.h"
#include "GreenKinectAdapter.h"
#pragma managed
using namespace std;
using namespace System;
using namespace System::Windows;
using namespace System::Windows::Interop;
using namespace System::Runtime::InteropServices;
using namespace System::Reflection;
using namespace System::IO;
using namespace Green::Kinect;
using Green::MathExtensions::Float4;

namespace Green
{
	namespace Graphics
	{
		public enum class SaveFormats {
			STL = 0,
			FBX,
			DXF,
			DAE,
			OBJ,
			FL4
		};

		public interface class IModelSaver
		{
			bool SaveModel(String^ path, SaveFormats format);
		};

		public ref class GraphicsCanvas : public HwndHost, IModelSaver
		{
		private:
			DirectXWindow* XWindow;
			HWND Host, Canvas;
			static GraphicsCanvas()
			{
				VertexDefinition::Init();
			}
		protected:
			virtual HandleRef BuildWindowCore(HandleRef hwndParent) override
			{
				HWND parent = (HWND)hwndParent.Handle.ToPointer();

				Host = nullptr;
				Host = CreateWindowEx(
					0, L"static", L"",
					WS_CHILD,
					0, 0, (int)Width, (int)Height,
					parent,
					0, nullptr, 0);
				String^ root = Path::GetDirectoryName(Assembly::GetExecutingAssembly()->Location);
				XWindow = new DirectXWindow(Host, StringToLPWSTR(root));
				return HandleRef(this, (IntPtr)Host);
			}

			virtual void DestroyWindowCore(HandleRef hwnd) override
			{
				XWindow = nullptr;
				delete XWindow;
				DestroyWindow((HWND)hwnd.Handle.ToPointer());
			}

			bool ResizeNeeded;
			virtual IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, bool %handled) override
			{
				static int resizeTimer = 0;
				switch (msg)
				{
				case WM_SIZE:
					resizeTimer = SetTimer((HWND)hwnd.ToPointer(), resizeTimer, 100, 0);
					ResizeNeeded = true;
					break;
				case WM_TIMER:
					if (ResizeNeeded)
					{
						XWindow->Resize();
						ResizeNeeded = false;
					}
					break;
				}
				handled = false;
				return IntPtr::Zero;
			}
		public:
			void SetKinectDevice(KinectManager^ manager)
			{
				XWindow->InitKinect(manager->Device);
			}

			GraphicsCanvas()
			{
				XWindow = nullptr;
			}

			~GraphicsCanvas()
			{
				delete XWindow;
			}

			DirectXWindow* GetDirectXWindow()
			{
				return XWindow;
			}

			bool GetVertices([Out] array<Float4, 2>^ %vertices)
			{
				int width, height;
				XMFLOAT4* data;
				if (XWindow->GetVertices(data, width, height))
				{
					int bufferLength = width * height;
					vertices = gcnew array<Float4, 2>(width, height);
					pin_ptr<Float4> sVertices = &vertices[0, 0];
					memcpy(sVertices, data, bufferLength * sizeof(XMFLOAT4));
					delete[bufferLength] data;
					return true;
				}
				else return false;
			}

			bool SaveImage(String^ path)
			{
				LPWSTR npath = StringToLPWSTR(path);
				bool ok = XWindow->SaveImage(npath);
				LPWSTRDelete(npath);
				return ok;
			}

			bool SaveRawImage(String^ path)
			{
				LPWSTR npath = StringToLPWSTR(path);
				bool ok = XWindow->SaveRawImage(npath);
				LPWSTRDelete(npath);
				return ok;
			}

			virtual bool SaveModel(String^ path, SaveFormats format)
			{
				LPWSTR npath = StringToLPWSTR(path);
				bool ok = XWindow->SaveModel(npath, (ModelFormats)format);
				LPWSTRDelete(npath);
				return ok;
			}

			void SetSave(int width, int height, int texWidth, int texHeight, float scaling)
			{
				XWindow->SetSave(width, height, texWidth, texHeight, scaling);
			}

			void SetView(
				float transX, float transY, float transZ,
				float rotX, float rotY, float rotZ,
				float scale, float moveX, float moveY, float rotation)
			{
				XWindow->SetView(transX, transY, transZ, rotX, rotY, rotZ, scale, moveX, moveY, rotation);
			}

			void SetCameras(
				array<float, 2>^ infraredIntrinsics, array<float, 2>^ infraredDistortion,
				bool infraredDistortionCorrectionEnabled, array<float, 2>^ depthToIRMapping,
				array<float, 2>^ colorIntrinsics, array<float, 2>^ colorRemapping,
				array<float, 2>^ colorExtrinsics, int colorDispX, int colorDispY,
				float colorScaleX, float colorScaleY, array<float, 2>^ depthCoeffs)
			{
				if (Is3x3(infraredIntrinsics) && Is3x3(depthToIRMapping))
				{
					pin_ptr<float> pInfraredIntrinsics = &To4x4(infraredIntrinsics)[0, 0];
					pin_ptr<float> pInfraredDistortion = &infraredDistortion[0, 0];
					pin_ptr<float> pDepthToIRMapping = &Expand4x4(depthToIRMapping)[0, 0];
					pin_ptr<float> pColorIntrinsics = &To4x4(colorIntrinsics)[0, 0];
					pin_ptr<float> pColorRemapping = &Expand4x4(colorRemapping)[0, 0];
					pin_ptr<float> pColorExtrinsics = &colorExtrinsics[0, 0];
					pin_ptr<float> pDepthCoeffs = &depthCoeffs[0, 0];
					XWindow->SetCameras(
						pInfraredIntrinsics, (infraredDistortionCorrectionEnabled ? pInfraredDistortion : nullptr),
						pDepthToIRMapping, pColorIntrinsics, pColorRemapping, pColorExtrinsics,
						colorDispX, colorDispY, colorScaleX, colorScaleY, pDepthCoeffs);
				}
			}

			void SetPreprocessing(int depthAveraging, int depthGaussIterations, float depthGaussSigma)
			{
				XWindow->SetPreprocessing(depthAveraging, depthGaussIterations, depthGaussSigma);
			}

			void SetPerformance(int triangleGridWidth, int triangleGridHeight)
			{
				XWindow->SetPerformance(triangleGridWidth, triangleGridHeight);
			}

			enum class ShadingModes {
				Zebra,
				Rainbow,
				ShadedRainbow,
				Scale,
				ShadedScale,
				Phong,
				Textured
			};

			void SetShading(ShadingModes mode, float depthMaximum, float depthMinimum, float shadingPeriode, float shadingPhase, float triangleLimit, bool wireframeShading, bool useModuleShading)
			{
				XWindow->SetShading((DirectXWindow::ShadingModes)mode, depthMaximum, depthMinimum, shadingPeriode, shadingPhase, triangleLimit, wireframeShading, useModuleShading);
			}

			void SetLighting(float ambient, float diffuse, float specular, float shininess)
			{
				XWindow->SetLighting(ambient, diffuse, specular, shininess);
			}

			void Draw()
			{
				if (XWindow != nullptr)
					XWindow->Draw();
			}
		};

	}
}

