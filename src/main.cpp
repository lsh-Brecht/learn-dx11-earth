#define NOMINMAX
#include <windows.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "user32") 
#pragma comment(lib, "d3d11") 
#pragma comment(lib, "d3dcompiler")

#include <d3d11.h>
#include <d3dcompiler.h>

/*
#include "cgmath.h"
#include "mouse.h"
*/

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
};

class URenderer
{
public:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;
	ID3D11Texture2D* DepthStencilBuffer = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11RasterizerState* RasterizerStateSolid = nullptr;
	ID3D11RasterizerState* RasterizerStateWireframe = nullptr;
	ID3D11Buffer* ConstantBuffer = nullptr;

	ID3D11ShaderResourceView* TextureView = nullptr;
	ID3D11SamplerState* SamplerState = nullptr;

	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
	D3D11_VIEWPORT ViewportInfo;

	ID3D11VertexShader* SimpleVertexShader = nullptr;
	ID3D11PixelShader* SimplePixelShader = nullptr;
	ID3D11InputLayout* SimpleInputLayout = nullptr;
	unsigned int Stride = sizeof(Vertex);

	struct FConstants
	{
		mat4 model_matrix;
		mat4 view_projection_matrix;

		vec4 light_position;
		vec4 Ia, Id, Is;
		vec4 Ka, Kd, Ks;

		float shininess;
		int mode;
		float pad1[2];

		vec3 camera_position;
		float pad2;
	};

public:
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);
		CreateFrameBuffer();
		CreateRasterizerState();
	}

	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };
		DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
		swapchaindesc.BufferDesc.Width = 0;
		swapchaindesc.BufferDesc.Height = 0;
		swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapchaindesc.SampleDesc.Count = 1;
		swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchaindesc.BufferCount = 2;
		swapchaindesc.OutputWindow = hWindow;
		swapchaindesc.Windowed = TRUE;
		swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
			&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

		SwapChain->GetDesc(&swapchaindesc);
		ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
	}

	void CreateFrameBuffer()
	{
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);

		D3D11_TEXTURE2D_DESC depthDesc = {};
		depthDesc.Width = (UINT)ViewportInfo.Width;
		depthDesc.Height = (UINT)ViewportInfo.Height;
		depthDesc.MipLevels = 1;
		depthDesc.ArraySize = 1;
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.SampleDesc.Quality = 0;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		Device->CreateTexture2D(&depthDesc, nullptr, &DepthStencilBuffer);
		Device->CreateDepthStencilView(DepthStencilBuffer, nullptr, &DepthStencilView);
	}

	void CreateRasterizerState()
	{
		D3D11_RASTERIZER_DESC rasterizerdesc = {};
		rasterizerdesc.CullMode = D3D11_CULL_BACK;

		rasterizerdesc.FrontCounterClockwise = TRUE;

		rasterizerdesc.FillMode = D3D11_FILL_SOLID;
		Device->CreateRasterizerState(&rasterizerdesc, &RasterizerStateSolid);

		rasterizerdesc.FillMode = D3D11_FILL_WIREFRAME;
		Device->CreateRasterizerState(&rasterizerdesc, &RasterizerStateWireframe);
	}

	void Release()
	{
		if (SamplerState) { SamplerState->Release(); SamplerState = nullptr; }
		if (TextureView) { TextureView->Release(); TextureView = nullptr; }
		if (RasterizerStateSolid) RasterizerStateSolid->Release();
		if (RasterizerStateWireframe) RasterizerStateWireframe->Release();

		if (FrameBufferRTV) FrameBufferRTV->Release();
		if (FrameBuffer) FrameBuffer->Release();
		if (SwapChain) SwapChain->Release();
		if (DeviceContext) DeviceContext->Release();
		if (Device) Device->Release();
	}

	void SwapBuffer()
	{
		SwapChain->Present(1, 0);
	}

	void CreateShader()
	{
		ID3DBlob* vertexshaderCSO;
		ID3DBlob* pixelshaderCSO;

		D3DCompileFromFile(L"./bin/Shader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);
		Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

		D3DCompileFromFile(L"./bin/Shader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);
		Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		Device->CreateInputLayout(layout, ARRAYSIZE(layout),
			vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(),
			&SimpleInputLayout);

		Stride = sizeof(Vertex);

		vertexshaderCSO->Release();
		pixelshaderCSO->Release();
	}

	void ReleaseShader()
	{
		if (SimpleInputLayout) SimpleInputLayout->Release();
		if (SimplePixelShader) SimplePixelShader->Release();
		if (SimpleVertexShader) SimpleVertexShader->Release();
	}

	void Prepare(bool bWireframe)
	{
		DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
		DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		DeviceContext->RSSetViewports(1, &ViewportInfo);

		DeviceContext->RSSetState(bWireframe ? RasterizerStateWireframe : RasterizerStateSolid);

		DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView);
	}

	void PrepareShader()
	{
		DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
		DeviceContext->IASetInputLayout(SimpleInputLayout);
		if (ConstantBuffer)
		{
			DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
			DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
		}
		if (TextureView && SamplerState)
		{
			//Connect TextureView to t0 slot in HLSL
			DeviceContext->PSSetShaderResources(0, 1, &TextureView);
			//Connect SamplerState to s0 slot in HLSL
			DeviceContext->PSSetSamplers(0, 1, &SamplerState);
		}
	}

	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
	{
		UINT offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
		DeviceContext->Draw(numVertices, 0);
	}

	ID3D11Buffer* CreateVertexBuffer(void* vertices, UINT byteWidth)
	{
		D3D11_BUFFER_DESC vertexbufferdesc = {};
		vertexbufferdesc.ByteWidth = byteWidth;
		vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };
		ID3D11Buffer* vertexBuffer;
		Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

		return vertexBuffer;
	}

	void ReleaseFrameBuffer()
	{
		if (DepthStencilView)
		{
			DepthStencilView->Release();
			DepthStencilView = nullptr;
		}
		if (DepthStencilBuffer)
		{
			DepthStencilBuffer->Release();
			DepthStencilBuffer = nullptr;
		}
	}

	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
	{
		if (vertexBuffer) vertexBuffer->Release();
	}

	void CreateConstantBuffer()
	{
		D3D11_BUFFER_DESC constantbufferdesc = {};
		constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;
		constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
		constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
	}

	void UpdateConstant(const FConstants& constantData)
	{
		if (ConstantBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);

			FConstants* constants = (FConstants*)constantbufferMSR.pData;
			{
				*constants = constantData;
			}

			DeviceContext->Unmap(ConstantBuffer, 0);
		}
	}

	void ReleaseConstantBuffer()
	{
		if (ConstantBuffer) ConstantBuffer->Release();
	}

	void CreateTextureAndSampler(const char* filepath)
	{
		int width, height, channels;
		unsigned char* image_data = stbi_load(filepath, &width, &height, &channels, 4);
		if (!image_data)
		{
			printf("Failed to load texture: %s\n", filepath);
			return;
		}

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = image_data;
		initData.SysMemPitch = width * 4;

		ID3D11Texture2D* pTexture = nullptr;
		Device->CreateTexture2D(&desc, &initData, &pTexture);

		stbi_image_free(image_data);

		Device->CreateShaderResourceView(pTexture, nullptr, &TextureView);
		pTexture->Release();

		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		Device->CreateSamplerState(&sampDesc, &SamplerState);
	}
};

std::vector<Vertex> create_sphere_vertices()
{
	std::vector<Vertex> v;
	std::vector<uint> indices;
	uint num_x = 72; uint num_y = 36;

	for (uint i = 0; i < num_y + 1; i++)
	{
		float theta = (PI * i) / float(num_y);
		float sin_theta = sin(theta); float cos_theta = cos(theta);
		for (uint j = 0; j < num_x + 1; j++)
		{
			float phi = (2.0f * PI * j) / float(num_x);
			float sin_phi = sin(phi); float cos_phi = cos(phi);
			float tx = (phi / (2.0f * PI));
			float ty = theta / PI;
			vec3 pos = vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
			v.push_back({ pos, pos, vec2(tx, ty) });
		}
	}

	int num_v_x = num_x + 1;
	for (uint i = 0; i < num_y; i++)
	{
		for (uint j = 0; j < num_x; j++)
		{
			uint k0 = i * num_v_x + j; uint k1 = (i + 1) * num_v_x + j;
			uint k2 = (i + 1) * num_v_x + (j + 1); uint k3 = i * num_v_x + (j + 1);
			indices.push_back(k0); indices.push_back(k1); indices.push_back(k3);
			indices.push_back(k1); indices.push_back(k2); indices.push_back(k3);
		}
	}

	std::vector<Vertex> result_vertices;
	for (uint idx : indices) result_vertices.push_back(v[idx]);
	return result_vertices;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY: PostQuitMessage(0); break;
	default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WCHAR WindowClassName[] = L"JungleWindowClass";
	WCHAR Title[] = L"JungleWindowClass";
	WNDCLASSW wndclass = { 0, WndProc, 0,0, 0,0,0,0,0, WindowClassName };
	RegisterClassW(&wndclass);
	HWND hWnd = CreateWindowExW(0, WindowClassName, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024, nullptr, nullptr, hInstance, nullptr);

	URenderer renderer;
	renderer.Create(hWnd);
	renderer.CreateShader();
	renderer.CreateConstantBuffer();
	renderer.CreateTextureAndSampler("earth.jpg");

	std::vector<Vertex> sphereVertices = create_sphere_vertices();
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphereVertices.data(), (UINT)(sizeof(Vertex) * sphereVertices.size()));
	UINT numVerticesSphere = (UINT)sphereVertices.size();

	bool bIsExit = false;
	bool b_rotating = true;
	float current_angle = 0.0f;
	int i_solid_color = 0;
	bool bWireframe = false;

	const int targetFPS = 30;
	const double targetFrameTime = 1000.0 / targetFPS;
	LARGE_INTEGER frequency, startTime, endTime;
	QueryPerformanceFrequency(&frequency);
	double elapsedTime = 0.0;

	camera cam;
	cam.eye = vec3(5.0f, 0.0f, 0.0f);
	cam.at = vec3(0.0f, 0.0f, 0.0f);
	cam.up = vec3(0.0f, 1.0f, 0.0f);
	cam.view_matrix = mat4::look_at(cam.eye, cam.at, cam.up);

	camera cam0 = cam;

	trackball tb(1.0f);

	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) { bIsExit = true; break; }

			if (msg.message == WM_KEYDOWN)
			{
				if (msg.wParam == 'Q') bIsExit = true;
				if (msg.wParam == 'D') i_solid_color = (i_solid_color + 1) % 3;
				if (msg.wParam == 'R') b_rotating = !b_rotating;
				if (msg.wParam == 'W') bWireframe = !bWireframe;
				if (msg.wParam == VK_HOME)
				{
					current_angle = 0.0f;
					cam = cam0;
				}
			}
			else if (msg.message == WM_LBUTTONDOWN)
			{
				int x = LOWORD(msg.lParam);
				int y = HIWORD(msg.lParam);

				RECT rect;
				GetClientRect(hWnd, &rect);
				ivec2 window_size = { rect.right - rect.left, rect.bottom - rect.top };

				vec2 npos = cursor_to_ndc(dvec2(x, y), window_size);
				tb.begin(cam, npos);
			}
			else if (msg.message == WM_LBUTTONUP)
			{
				tb.end();
			}
			else if (msg.message == WM_MOUSEMOVE)
			{
				if (tb.is_tracking())
				{
					int x = LOWORD(msg.lParam);
					int y = HIWORD(msg.lParam);

					RECT rect;
					GetClientRect(hWnd, &rect);
					ivec2 window_size = { rect.right - rect.left, rect.bottom - rect.top };

					vec2 npos = cursor_to_ndc(dvec2(x, y), window_size);
					cam = tb.update(npos);
				}
			}
		}

		//////////////////

		renderer.Prepare(bWireframe);
		renderer.PrepareShader();

		RECT rect;
		GetClientRect(hWnd, &rect);
		float clientWidth = (float)(rect.right - rect.left);
		float clientHeight = (float)(rect.bottom - rect.top);
		ivec2 window_size = { (int)clientWidth, (int)clientHeight };
		POINT pt;
		GetCursorPos(&pt); ScreenToClient(hWnd, &pt);
		dvec2 cursor_pos = { (double)pt.x, (double)pt.y };
		vec2 ndc_pos = cursor_to_ndc(cursor_pos, window_size);
		float aspect = clientWidth / clientHeight;

		mat4 projection_matrix = mat4::perspective(cam.fovy, aspect, cam.dnear, cam.dfar);

		// ---------------------------------
		// OpenGL(-1 ~ 1)¸¦ Direct3D(0 ~ 1)
		// ---------------------------------
		projection_matrix._33 = cam.dfar / (cam.dnear - cam.dfar);
		projection_matrix._34 = (cam.dnear * cam.dfar) / (cam.dnear - cam.dfar);

		mat4 view_projection = projection_matrix * cam.view_matrix;

		if (b_rotating) current_angle += (float)(elapsedTime / 1000.0f) * 0.5f;
		float c = cos(current_angle), s = sin(current_angle);
		mat4 anim_matrix = {
			c, -s, 0, 0,
			s,  c, 0, 0,
			0,  0, 1, 0,
			0,  0, 0, 1
		};
		mat4 rotation_matrix = {
			0, 1, 0, 0,
			0, 0, 1, 0,
			1, 0, 0, 0,
			0, 0, 0, 1
		};
		mat4 model_matrix = rotation_matrix * anim_matrix;

		URenderer::FConstants constantData;
		constantData.model_matrix = model_matrix.transpose();
		constantData.view_projection_matrix = view_projection.transpose();

		constantData.light_position = vec4(5.0f, 0.0f, 0.0f, 1.0f);

		constantData.Ia = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		constantData.Id = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		constantData.Is = vec4(1.0f, 1.0f, 1.0f, 1.0f);

		constantData.Ka = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		constantData.Kd = vec4(0.7f, 0.7f, 0.7f, 1.0f);
		constantData.Ks = vec4(0.8f, 0.8f, 0.8f, 1.0f);
		constantData.shininess = 256.0f;

		constantData.camera_position = cam.eye;

		constantData.mode = i_solid_color;

		renderer.UpdateConstant(constantData);

		renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);

		renderer.SwapBuffer();

		do
		{
			Sleep(0);
			QueryPerformanceCounter(&endTime);
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;
		} while (elapsedTime < targetFrameTime);
	}

	renderer.ReleaseFrameBuffer();
	renderer.ReleaseVertexBuffer(vertexBufferSphere);
	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.Release();

	return 0;
}