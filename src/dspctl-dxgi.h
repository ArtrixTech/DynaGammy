/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef DXGIDUPL_H
#define DXGIDUPL_H

#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <stdint.h>
#include <vector>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "Advapi32.lib")

namespace GDI {
const static HDC screenDC = GetDC(nullptr);
const static uint64_t w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN);
const static uint64_t h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);
void setGamma(int, int);
void getSnapshot(std::vector<uint8_t> &buf);
}

class DspCtl
{
public:
	DspCtl();
	~DspCtl();

	void setGamma(int, int);
	void getSnapshot(std::vector<uint8_t> &buf) noexcept;
	void setInitialGamma(bool);
private:
	ID3D11Device* d3d_device;
	ID3D11DeviceContext* d3d_context;
	IDXGIOutput1* output1;
	IDXGIOutputDuplication* duplication;
	D3D11_TEXTURE2D_DESC    tex_desc;
	bool useDXGI;
	
	bool init();
	bool getFrame(std::vector<uint8_t> &buf);
	void restart();
};

#endif // DXGIDUPL_H

