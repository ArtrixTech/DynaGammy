/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "dspctl-dxgi.h"
#include "defs.h"
#include "cfg.h"
#include "utils.h"

GDI::GDI()
{

}

GDI::~GDI()
{
	for (const auto& hdc : hdcs) {
		DeleteDC(hdc);
	}
}

int GDI::numDisplays()
{
	DISPLAY_DEVICE dsp;
	dsp.cb = sizeof(DISPLAY_DEVICE);
	int i = 0;
	int attached_dsp = 0;
	while (EnumDisplayDevices(NULL, i++, &dsp, 0) && dsp.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
		attached_dsp++;
	return attached_dsp;
}

void GDI::createDCs(const std::wstring &primary_screen_name)
{
	const int num_dsp = numDisplays();
	hdcs.reserve(num_dsp);

	for (int i = 0; i < num_dsp; ++i) {
		DISPLAY_DEVICE dsp;
		dsp.cb = sizeof(DISPLAY_DEVICE);
		EnumDisplayDevices(NULL, i, &dsp, 0);

		HDC dc = CreateDC(NULL, dsp.DeviceName, NULL, 0);
		hdcs.push_back(dc);

		// If we don't have the name, just pick the first one.
		if ((i == 0 && primary_screen_name.empty()) || dsp.DeviceName == primary_screen_name) {
			primary_dc_idx = i;
			width  = GetDeviceCaps(dc, HORZRES);
			height = GetDeviceCaps(dc, VERTRES);
			buf.resize(width * height * 4);
			setBitmapInfo(width, height);
		}
	}

	LOGD << "HDCs: " << hdcs.size();
	LOGD << "GDI res: " << width << '*' << height;
}

void GDI::setGamma(int brt_step, int temp_step)
{
	const double r_mult = interpTemp(temp_step, 0),
	             g_mult = interpTemp(temp_step, 1),
	             b_mult = interpTemp(temp_step, 2);

	WORD ramp[3][256];
	const double brt_mult = remap(brt_step, 0, brt_steps_max, 0, 255);

	for (WORD i = 0; i < 256; ++i) {
		const int val = i * brt_mult;
		ramp[0][i] = WORD(val * r_mult);
		ramp[1][i] = WORD(val * g_mult);
		ramp[2][i] = WORD(val * b_mult);
	}

	/* As auto brt is currently supported only on the primary screen,
	 * We set this ramp to the screens whose image brightness is not controlled. */
	WORD ramp_full_brt[3][256];
	if (hdcs.size() > 1) {
		for (WORD i = 0; i < 256; ++i) {
			const int val = i * 255;
			ramp_full_brt[0][i] = WORD(val * r_mult);
			ramp_full_brt[1][i] = WORD(val * g_mult);
			ramp_full_brt[2][i] = WORD(val * b_mult);
		}
	}

	int i = 0;
	for (const auto &dc : hdcs) {
		bool r;
		if (i == primary_dc_idx)
			r = SetDeviceGammaRamp(dc, ramp);
		else
			r = SetDeviceGammaRamp(dc, ramp_full_brt);

		LOGV << "screen " << i <<  " gamma set: " << r;
		i++;
	}
}

void GDI::setInitialGamma([[maybe_unused]] bool set_previous)
{
	// @TODO: restore previous gamma
	GDI::setGamma(brt_steps_max, 0);
}

void GDI::setBitmapInfo(int width, int height)
{
	ZeroMemory(&info, sizeof(BITMAPINFOHEADER));
	info.biSize         = sizeof(BITMAPINFOHEADER);
	info.biWidth        = width;
	info.biHeight       = -height;
	info.biPlanes       = 1;
	info.biBitCount     = 32;
	info.biCompression  = BI_RGB;
	info.biSizeImage    = width * height * 4;
	info.biClrUsed      = 0;
	info.biClrImportant = 0;
}

int GDI::getScreenBrightness() noexcept
{
	HDC     dc  = GetDC(NULL);
	HBITMAP bmp = CreateCompatibleBitmap(dc, width, height);
	HDC     tmp = CreateCompatibleDC(dc);
	HGDIOBJ obj = SelectObject(tmp, bmp);

	BitBlt(tmp, 0, 0, width, height, dc, 0, 0, SRCCOPY);
	GetDIBits(tmp, bmp, 0, height, buf.data(), LPBITMAPINFO(&info), DIB_RGB_COLORS);

	SelectObject(tmp, obj);
	DeleteObject(bmp);
	DeleteObject(obj);
	DeleteDC(tmp);
	DeleteDC(dc);

	return calcBrightness(buf.data(), buf.size(), 4, 1024);
}

DXGI::DXGI()
{
	if (!(useDXGI = init())) {
		LOGD << "DXGI unavailable.";
		primary_screen_name.clear();
	}
	
	GDI::createDCs(this->primary_screen_name);
}

DXGI::~DXGI()
{
	//if (useDXGI)
	//	destroy();
}

bool DXGI::init()
{
	std::vector<IDXGIAdapter1*> gpus;
	std::vector<IDXGIOutput*>   outputs;
	// Retrieve a IDXGIFactory to enumerate the adapters
	IDXGIFactory1 *factory = nullptr;
	if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)) != S_OK) {
		LOGE << "CreateDXGIFactory1 failed";
		return false;
	}

	IDXGIAdapter1 *tmp;
	int i = 0;
	while (factory->EnumAdapters1(i++, &tmp) != DXGI_ERROR_NOT_FOUND) {
		gpus.push_back(tmp);
	}
	factory->Release();

	// Get the monitors attached to the GPUs
	for (size_t i = 0; i < gpus.size(); ++i) {
		IDXGIOutput *output;
		for (int j = 0; gpus[i]->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND; ++j) {
			LOGD << "Found monitor " << j << " on GPU " << i;
			outputs.push_back(output);
		}
	}

	if (outputs.empty()) {
		LOGE << "No outputs found";
		return false;
	}

	// Get GPU info
	for (size_t i = 0; i < gpus.size(); ++i) {
		DXGI_ADAPTER_DESC1 desc;
		if (gpus[i]->GetDesc1(&desc) == S_OK)
			LOGD << "GPU " << i << ": " << desc.Description;
		else
			LOGE << "Failed to get desc for GPU " << i;
	}

	// Get screen info and primary screen name
	for (size_t i = 0; i < outputs.size(); ++i) {
		DXGI_OUTPUT_DESC desc;
		if (outputs[i]->GetDesc(&desc) != S_OK) {
			LOGE << "Failed to get description for output " << i;
			continue;
		}
		LOGD << "Output: " << desc.DeviceName << ", attached to desktop: " << desc.AttachedToDesktop;	
		if (i == 0)
			primary_screen_name = desc.DeviceName;
	}

	D3D_FEATURE_LEVEL d3d_feature_level;
	HRESULT hr = D3D11CreateDevice(
				gpus[0],                  // Adapter: The adapter (video card) we want to use. We may use NULL to pick the default adapter.
				D3D_DRIVER_TYPE_UNKNOWN,  // DriverType: We use the GPU as backing device.
				nullptr,                  // Software: we're using a D3D_DRIVER_TYPE_HARDWARE so it's not applicable.
				NULL,                     // Flags: maybe we need to use D3D11_CREATE_DEVICE_BGRA_SUPPORT because desktop duplication is using this.
				nullptr,                  // Feature Levels:  what version to use.
				0,                        // Number of feature levels.
				D3D11_SDK_VERSION,        // The SDK version, use D3D11_SDK_VERSION
				&d3d_device,              // OUT: the ID3D11Device object.
				&d3d_feature_level,       // OUT: the selected feature level.
	                        &d3d_context);            // OUT: the ID3D11DeviceContext that represents the above features.

	d3d_context->Release();

	for (const auto &gpu : gpus)
		gpu->Release();

	switch (hr) {
	case S_OK:
		break;
	case E_INVALIDARG:
		LOGE << "D3D11CreateDevice: E_INVALIDARG";
		[[fallthrough]]; 
	default:
		return false;
	}

	// Currently, auto brightness is only supported on the first screen.
	const int output_idx = 0;

	// Set texture properties
	DXGI_OUTPUT_DESC output_desc;
	if (outputs[output_idx]->GetDesc(&output_desc) != S_OK) {
		LOGE << "Failed to get description for screen " << output_idx;
		return false;
	}

	tex_desc.Width              = output_desc.DesktopCoordinates.right;
	tex_desc.Height             = output_desc.DesktopCoordinates.bottom;
	tex_desc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM; // 4 bits per pixel
	tex_desc.Usage              = D3D11_USAGE_STAGING;
	tex_desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
	tex_desc.MipLevels          = 1;
	tex_desc.ArraySize          = 1;
	tex_desc.SampleDesc.Count   = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.BindFlags          = 0;
	tex_desc.MiscFlags          = 0;
	LOGD << "DXGI res: " << tex_desc.Width << "*" << tex_desc.Height;

	// Initialize output duplication
	if (outputs[output_idx]->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&output1)) != S_OK) {
		LOGE << "Failed to query IDXGIOutput1 interface";
		return false;
	}

	if (output1->DuplicateOutput(d3d_device, &duplication) != S_OK) {
		LOGE << "DuplicateOutput failed";
		return false;
	}

	output1->Release();
	d3d_device->Release();

	for (const auto &output : outputs)
		output->Release();

	return true;
}

void DXGI::destroy()
{
	if (duplication) {
		duplication->ReleaseFrame();
		duplication->Release();
	}
}

int  DXGI::getScreenBrightness()
{
	if (!useDXGI) {
		Sleep(cfg["brt_polling_rate"].get<int>());
		return GDI::getScreenBrightness();
	}

	IDXGIResource           *desktop_res;
	DXGI_OUTDUPL_FRAME_INFO frame_info;

	if (HRESULT hr = duplication->AcquireNextFrame(INFINITE, &frame_info, &desktop_res) != S_OK) {
		LOGD << "AcquireNextFrame failed (" << hr << ").";
		Sleep(2500);
		restart();
		return 255;
	}

	ID3D11Texture2D *tex;
	desktop_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
	desktop_res->Release();
	tex->Release();

	ID3D11Texture2D *staging_tex;
	d3d_device->CreateTexture2D(&this->tex_desc, nullptr, &staging_tex);
	d3d_context->CopyResource(staging_tex, tex);
	duplication->ReleaseFrame();

	D3D11_MAPPED_SUBRESOURCE map;

	while (d3d_context->Map(staging_tex, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &map) == DXGI_ERROR_WAS_STILL_DRAWING)
		Sleep(cfg["brt_polling_rate"].get<int>());

	d3d_context->Unmap(staging_tex, 0);
	staging_tex->Release();
	d3d_context->Release();

	return calcBrightness(reinterpret_cast<uint8_t*>(map.pData), map.DepthPitch, 4, 1024);
}

void DXGI::restart()
{
	LOGD << "Releasing duplication...";
	duplication->ReleaseFrame();
	duplication->Release();
	LOGD << "Restarting duplication...";

	switch (output1->DuplicateOutput(d3d_device, &duplication)) {
	case S_OK:
		LOGD << "Restarted successfully.";
		break;
	case E_INVALIDARG:
		LOGE << "E_INVALIDARG";
		break;
	case E_ACCESSDENIED:
		LOGE << "E_ACCESSDENIED";
		break;
	case DXGI_ERROR_UNSUPPORTED:
		LOGE << "E_DXGI_ERROR_UNSUPPORTED";
		break;
	case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
		LOGE << "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
		break;
	case DXGI_ERROR_SESSION_DISCONNECTED:
		LOGE << "DXGI_ERROR_SESSION_DISCONNECTED";
		break;
	}

	output1->Release();
}
