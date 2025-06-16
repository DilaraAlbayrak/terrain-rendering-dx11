#include "D3DFramework.h"
#include <directxcolors.h>
#include <vector>
#include "Resource.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <unordered_map>

// to import .png
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::unique_ptr<D3DFramework> D3DFramework::_instance = std::make_unique<D3DFramework>();


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK D3DFramework::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message) {
	case WM_LBUTTONDOWN:
		_instance->_isMouseDown = true;
		GetCursorPos(&_instance->_lastMousePos);
		break;
	case WM_LBUTTONUP:
		_instance->_isMouseDown = false;
		break;
	//case WM_MOUSEMOVE:
	//	if (_instance->_isMouseDown)
	//	{
	//		POINT currentMousePos;
	//		GetCursorPos(&currentMousePos);
	//		
	//		float dx = (currentMousePos.x - _instance->_lastMousePos.x) * _instance->_sensitivity;
	//		float dy = (currentMousePos.y - _instance->_lastMousePos.y) * _instance->_sensitivity;

	//		_instance->_yaw += dx;
	//		_instance->_pitch -= dy;

	//		float epsilon = 0.01f;
	//		if (_instance->_pitch > XM_PIDIV2 - epsilon) _instance->_pitch = XM_PIDIV2 - epsilon;
	//		if (_instance->_pitch < -XM_PIDIV2 + epsilon) _instance->_pitch = -XM_PIDIV2 + epsilon;
	//		
	//		_instance->_camera.rotate(_instance->_yaw, _instance->_pitch);

	//		_instance->_lastMousePos = currentMousePos;
	//	}
	//	break;

	case WM_MOUSEWHEEL:
	{
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		float zoomAmount = 1.0f;
		if (wheelDelta > 0)  // Scroll up, Zoom in
			zoomAmount += _instance->_zoomSensitivity;
		else if (wheelDelta < 0) // Scroll down, Zoom out
			zoomAmount -= _instance->_zoomSensitivity;

		_instance->_camera.zoom *= zoomAmount;
		_instance->_camera.updateViewProjection();
		break;
	}

	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT D3DFramework::initWindow(HINSTANCE hInstance, int nCmdShow) {
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = wndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, reinterpret_cast<LPCTSTR>(IDI_SIMULATION));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"Starter Template";
	wcex.hIconSm = LoadIcon(wcex.hInstance, reinterpret_cast<LPCTSTR>(IDI_SIMULATION));
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	_hInst = hInstance;
	RECT rc = { 0, 0, _windowWidth, _windowHeight };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	_hWnd = CreateWindow(L"Starter Template", L"Moon Terrain Dx11",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!_hWnd)
		return E_FAIL;

	ShowWindow(_hWnd, nCmdShow);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT D3DFramework::initDevice()
{
	HRESULT hr = static_cast<HRESULT>(S_OK);

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	auto numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	auto numFeatureLevels = static_cast<UINT>(ARRAYSIZE(featureLevels));

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex) {
		_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(
			nullptr, _driverType, nullptr, createDeviceFlags,
			featureLevels, numFeatureLevels, D3D11_SDK_VERSION,
			&_pd3dDevice, &_featureLevel, &_pImmediateContext);

		if (hr == static_cast<HRESULT>(E_INVALIDARG)) {
			hr = D3D11CreateDevice(
				nullptr, _driverType, nullptr, createDeviceFlags,
				&featureLevels[1], numFeatureLevels - 1, D3D11_SDK_VERSION,
				&_pd3dDevice, &_featureLevel, &_pImmediateContext);
		}

		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr)) return hr;

	SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(1) << 1);

	CComPtr<IDXGIFactory1> dxgiFactory;
	{
		CComPtr<IDXGIDevice> dxgiDevice;
		hr = _pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr)) {
			CComPtr<IDXGIAdapter> adapter;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr)) {
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
			}
		}
	}
	if (FAILED(hr)) return hr;

	CComPtr<IDXGIFactory2> dxgiFactory2;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));

	hr = _pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&_pd3dDevice1));
	if (SUCCEEDED(hr)) {
		static_cast<void>(_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&_pImmediateContext1)));
	}

	DXGI_SWAP_CHAIN_DESC1 sd{};
	sd.Width = _windowWidth;
	sd.Height = _windowHeight;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;

	hr = dxgiFactory2->CreateSwapChainForHwnd(_pd3dDevice, _hWnd, &sd, nullptr, nullptr, &_swapChain1);
	if (SUCCEEDED(hr)) {
		hr = _swapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&_swapChain));
	}

	dxgiFactory->MakeWindowAssociation(_hWnd, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) return hr;

	CComPtr<ID3D11Texture2D> pBackBuffer;
	hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr)) return hr;

	hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
	if (FAILED(hr)) return hr;

	_pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView.p, nullptr);

	D3D11_VIEWPORT vp;
	vp.Width = static_cast<FLOAT>(_windowWidth);
	vp.Height = static_cast<FLOAT>(_windowHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	_pImmediateContext->RSSetViewports(1, &vp);

	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.ByteWidth = sizeof(ConstantBufferCamera);
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = _pd3dDevice->CreateBuffer(&cbDesc, nullptr, &_cameraConstantBuffer);
	if (FAILED(hr)) return hr;

	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width = _windowWidth;
	depthStencilDesc.Height = _windowHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	hr = _pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &_depthStencilBuffer);
	if (FAILED(hr)) return hr;

	hr = _pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);
	if (FAILED(hr)) return hr;

	CComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

	hr = D3DCompileFromFile(L"Simulation.fx", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		throw std::runtime_error("Vertex shader compile error");
	}

	hr = _pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_pVertexShader);
	if (FAILED(hr)) throw std::runtime_error("CreateVertexShader failed");

	hr = D3DCompileFromFile(L"Simulation.fx", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		throw std::runtime_error("Pixel shader compile error");
	}

	hr = _pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_pPixelShader);
	if (FAILED(hr)) throw std::runtime_error("CreatePixelShader failed");

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	hr = _pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_pVertexLayout);
	if (FAILED(hr)) throw std::runtime_error("CreateInputLayout failed");

	_diffuseSRV.Attach(loadPngTexture(L"textures/diffuse_map_8k.png"));

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = _pd3dDevice->CreateSamplerState(&sampDesc, &_samplerState);
	if (FAILED(hr)) throw std::runtime_error("Failed to create sampler state");

	loadModel("models/moon_low_res.obj");

	_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	initImGui();
	return S_OK;
}


void D3DFramework::initImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(_hWnd);
	ImGui_ImplDX11_Init(_pd3dDevice, _pImmediateContext);
}

void D3DFramework::renderImGui() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("FPS");


	ImGuiIO& io = ImGui::GetIO();
	float fps = io.Framerate;
	ImGui::Text("FPS: %.1f", fps);

	/*float colour[3] = { _bgColour.x, _bgColour.y, _bgColour.z };

	ImGui::Text("Set background colour:");
	if (ImGui::ColorEdit3("Colour", colour))
	{
		DirectX::XMFLOAT4 newColour = { colour[0], colour[1], colour[2], 1.0f };
		{
			_bgColour = newColour;
		}
	}*/
	ImGui::End();
	
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void D3DFramework::loadModel(const std::string& filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str()))
	{
		throw std::runtime_error("Failed to load model: " + warn + err);
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::unordered_map<std::string, uint32_t> uniqueVertices;

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if (index.normal_index >= 0)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (index.texcoord_index >= 0)
			{
				vertex.texcoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // flip V
				};
			}

			std::string key = std::to_string(vertex.position.x) + "," + std::to_string(vertex.position.y) + "," + std::to_string(vertex.position.z) +
				"," + std::to_string(vertex.normal.x) + "," + std::to_string(vertex.normal.y) + "," + std::to_string(vertex.normal.z) +
				"," + std::to_string(vertex.texcoord.x) + "," + std::to_string(vertex.texcoord.y);

			if (uniqueVertices.count(key) == 0)
			{
				uniqueVertices[key] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[key]);
		}
	}

	// Vertex buffer
	D3D11_BUFFER_DESC vbDesc{};
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(Vertex));
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vbData{};
	vbData.pSysMem = vertices.data();

	HRESULT hr = _pd3dDevice->CreateBuffer(&vbDesc, &vbData, &_pVertexBuffer);
	if (FAILED(hr)) throw std::runtime_error("Failed to create vertex buffer");

	// Index buffer
	D3D11_BUFFER_DESC ibDesc{};
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA ibData{};
	ibData.pSysMem = indices.data();

	hr = _pd3dDevice->CreateBuffer(&ibDesc, &ibData, &_pIndexBuffer);
	if (FAILED(hr)) throw std::runtime_error("Failed to create index buffer");

	_indexCount = static_cast<UINT>(indices.size());
}

ID3D11ShaderResourceView* D3DFramework::loadPngTexture(const std::wstring& filename)
{
	using namespace Gdiplus;

	Bitmap bitmap(filename.c_str());
	if (bitmap.GetLastStatus() != Ok)
		throw std::runtime_error("Failed to load PNG image");

	UINT width = bitmap.GetWidth();
	UINT height = bitmap.GetHeight();

	std::vector<UINT> pixelData(width * height);

	for (UINT y = 0; y < height; ++y)
	{
		for (UINT x = 0; x < width; ++x)
		{
			Color color;
			bitmap.GetPixel(x, y, &color);
			pixelData[y * width + x] = color.GetValue(); 
		}
	}

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pixelData.data();
	initData.SysMemPitch = width * 4;

	CComPtr<ID3D11Texture2D> texture;
	HRESULT hr = _pd3dDevice->CreateTexture2D(&texDesc, &initData, &texture);
	if (FAILED(hr)) throw std::runtime_error("Failed to create texture from image data");

	ID3D11ShaderResourceView* srv = nullptr;
	hr = _pd3dDevice->CreateShaderResourceView(texture, nullptr, &srv);
	if (FAILED(hr)) throw std::runtime_error("Failed to create shader resource view from texture");

	return srv;
}

D3DFramework::D3DFramework()
{
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
D3DFramework::~D3DFramework()
{
	GdiplusShutdown(gdiplusToken);
	try {
		if (_pImmediateContext)
			_pImmediateContext->ClearState();
	}
	catch (...) {

	}
}

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void D3DFramework::render()
{
	_pImmediateContext->OMSetDepthStencilState(_depthStencilState, 1);
	_pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView.p, _depthStencilView.p);

	float clearColour[4] = { _bgColour.x, _bgColour.y, _bgColour.z, _bgColour.w };
	_pImmediateContext->ClearRenderTargetView(_pRenderTargetView, clearColour);
	_pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	static auto start = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float>(now - start).count();

	DirectX::XMMATRIX modelRotation = XMMatrixRotationZ(time * 0.1f * XMConvertToRadians(90.0f));

	const ConstantBufferCamera cbc{
	XMMatrixTranspose(modelRotation),                     
	XMMatrixTranspose(_camera.view),              
	XMMatrixTranspose(_camera.projection),        
	{
		XMVectorGetX(_camera.eye),
		XMVectorGetY(_camera.eye),
		XMVectorGetZ(_camera.eye),
		0.0f
	}
	};

	_pImmediateContext->UpdateSubresource(_cameraConstantBuffer, 0, nullptr, &cbc, 0, 0);
	_pImmediateContext->VSSetConstantBuffers(0, 1, &_cameraConstantBuffer.p);

	// render .obj
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	_pImmediateContext->IASetInputLayout(_pVertexLayout);
	_pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBuffer.p, &stride, &offset);
	_pImmediateContext->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	_pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
	_pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
	_pImmediateContext->PSSetShaderResources(0, 1, &_diffuseSRV.p); 
	_pImmediateContext->PSSetSamplers(0, 1, &_samplerState.p);    

	_pImmediateContext->DrawIndexed(_indexCount, 0, 0);

	//renderImGui();
	_swapChain->Present(0, 0);
}
