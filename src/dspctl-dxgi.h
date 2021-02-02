/**
 * Copyright (C) Francesco Fusco. All rights reserved.
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
#include <string>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "Advapi32.lib")

class GDI
{
public:
	GDI();
	~GDI();

	int  getScreenBrightness() noexcept;
	void setGamma(int brt, int temp);
	void setInitialGamma(bool set_previous);
protected:
	void createDCs(const std::wstring &primary_screen_name);
private:
	std::vector<HDC> hdcs;
	BITMAPINFOHEADER info;
	std::vector<uint8_t> buf;

	/* In GDI, the index of the primary screen is not always 0, unlike DXGI.
	 * We get the proper index by comparing the GDI output names with the first DXGI output name. */
	int primary_dc_idx = 0;
	int width;
	int height;

	int numDisplays();
	void setBitmapInfo(int width, int height);
};

class DXGI : public GDI
{
public:
	DXGI();
	~DXGI();

	int getScreenBrightness();
private:
	ID3D11Device*           d3d_device;
	ID3D11DeviceContext*    d3d_context;
	IDXGIOutput1*           output1;
	IDXGIOutputDuplication* duplication;
	D3D11_TEXTURE2D_DESC    tex_desc;
	std::wstring            primary_screen_name;

	bool useDXGI;
	bool init();
	void destroy();
	void restart();
};

typedef DXGI DspCtl;

#endif // DXGIDUPL_H

