/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */


#include "dxgidupl.h"
#include <thread>
#include <chrono>
#include <iostream>

#include "utils.h"
#include "cfg.h"
#include "defs.h"

#include <locale>
#include <codecvt>
#define wchar_to_str(X) std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(X)

DXGIDupl::DXGIDupl() {}

bool DXGIDupl::initDXGI()
{
    IDXGIOutput                 *output;
    IDXGIAdapter1               *pAdapter;
    std::vector<IDXGIOutput*>   vOutputs;   // Monitors vector
    std::vector<IDXGIAdapter1*> vAdapters;  // GPUs vector

    // Retrieve a IDXGIFactory to enumerate the adapters
    {
        IDXGIFactory1 *pFactory = nullptr;
        HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory));

        if (hr != S_OK)
        {
            LOGE << "Failed to retrieve the IDXGIFactory";

            return false;
        }

        UINT i = 0;

        while (pFactory->EnumAdapters1(i++, &pAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            vAdapters.push_back(pAdapter);
        }

        pFactory->Release();

        IF_PLOG(plog::debug) // Get GPU info
        {
            DXGI_ADAPTER_DESC1 desc;

            for (UINT i = 0; i < vAdapters.size(); ++i)
            {
                pAdapter = vAdapters[i];
                HRESULT hr = pAdapter->GetDesc1(&desc);

                if (hr != S_OK)
                {
                    LOGE << "Failed to get description for adapter: " << i;
                    continue;
                }

                LOGD << "Adapter " << i << ": " << desc.Description;
            }
        }
    }

    // Get the monitors attached to the GPUs
    {
        UINT j;

        for (UINT i = 0; i < vAdapters.size(); ++i)
        {
            j = 0;
            pAdapter = vAdapters[i];

            while (pAdapter->EnumOutputs(j++, &output) != DXGI_ERROR_NOT_FOUND)
            {
                LOGD << "Found monitor " << j << " on adapter " << i;
                vOutputs.push_back(output);
            }
        }

        if (vOutputs.empty())
        {
            LOGE << "No outputs found";
            return false;
        }

        // Print monitor info
        DXGI_OUTPUT_DESC desc;

        for (size_t i = 0; i < vOutputs.size(); ++i)
        {
            output = vOutputs[i];
            HRESULT hr = output->GetDesc(&desc);

            if (hr != S_OK)
            {
                LOGE << "Failed to get description for output " << i;
                continue;
            }

            LOGD << "Monitor: " << desc.DeviceName << ", attached to desktop: " << desc.AttachedToDesktop;
        }
    }

    // Create a Direct3D device to access the OutputDuplication interface
    {
        D3D_FEATURE_LEVEL d3d_feature_level;
        IDXGIAdapter1 *d3d_adapter;
        UINT use_adapter = 0;

        if (vAdapters.size() <= use_adapter)
        {
            LOGE << "Invalid adapter index: " << use_adapter << ", we only have: " << vAdapters.size();
            return false;
        }

        d3d_adapter = vAdapters[use_adapter];

        if (!d3d_adapter)
        {
            LOGE << "The stored adapter is nullptr";
            return false;
        }

        HRESULT hr = D3D11CreateDevice(
            d3d_adapter,			  // Adapter: The adapter (video card) we want to use. We may use NULL to pick the default adapter.
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
        d3d_adapter->Release();

        if (hr != S_OK)
        {
            LOGE << "Failed to create D3D11 Device";

            if (hr == E_INVALIDARG)
            {
                LOGE << "Got INVALID arg passed into D3D11CreateDevice. Did you pass a adapter + a driver which is not the UNKNOWN driver?";
            }

            return false;
        }
    }

    // Choose what monitor to use
    {
        UINT use_monitor = 0;
        output = vOutputs[use_monitor];

        if (vOutputs.size() <= use_monitor)
        {
            LOGE << "Invalid monitor index";
            return false;
        }

        if (!output)
        {
            LOGE << "No valid output found. The output is nullptr";
            return false;
        }
    }

    // Set texture properties
    {
        DXGI_OUTPUT_DESC desc;
        HRESULT hr = output->GetDesc(&desc);

        if (hr != S_OK)
        {
            LOGE << "Failed to get output description";
            return false;
        }

        tex_desc.Width = desc.DesktopCoordinates.right;
        tex_desc.Height = desc.DesktopCoordinates.bottom;
        tex_desc.MipLevels = 1;
        tex_desc.ArraySize = 1;
        tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        tex_desc.SampleDesc.Count = 1;
        tex_desc.SampleDesc.Quality = 0;
        tex_desc.Usage = D3D11_USAGE_STAGING;
        tex_desc.BindFlags = 0;
        tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        tex_desc.MiscFlags = 0;
    }

    // Initialize output duplication
    {
        HRESULT hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&output1));

        if (hr != S_OK)
        {
            LOGE << "Failed to query IDXGIOutput1 interface";
            return false;
        }

        hr = output1->DuplicateOutput(d3d_device, &duplication);

        if (hr != S_OK)
        {
            LOGE << "DuplicateOutput failed";
            return false;
        }

        output1->Release();
        d3d_device->Release();
    }

    for (auto adapter : vAdapters) adapter->Release();
    for (auto output : vOutputs) output->Release();

    return true;
}

bool DXGIDupl::getDXGISnapshot(std::vector<uint8_t> &buf) noexcept
{
    HRESULT hr;

    ID3D11Texture2D *tex;
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    IDXGIResource *desktop_resource;

    hr = duplication->AcquireNextFrame(INFINITE, &frame_info, &desktop_resource);

    switch(hr)
    {
        case S_OK:
        {
            // Get the texture interface
            hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));

            desktop_resource->Release();
            tex->Release();

            if (hr != S_OK)
            {
                LOGE << "Failed to query the ID3D11Texture2D interface on the IDXGIResource";
                return false;
            }

            break;
        }
        case DXGI_ERROR_ACCESS_LOST:
        {
            LOGE << "Received a DXGI_ERROR_ACCESS_LOST";
            return false;
        }
        case DXGI_ERROR_WAIT_TIMEOUT:
        {
            LOGE << "Received a DXGI_ERROR_WAIT_TIMEOUT";
            return false;
        }
        default:
        {
            LOGE << "Error: failed to acquire frame";
            return false;
        }
     }

    ID3D11Texture2D *staging_tex;
    hr = d3d_device->CreateTexture2D(&tex_desc, nullptr, &staging_tex);

    if (hr != S_OK)
    {
        LOGE << "Texture creation failed, error: " << hr;
        return false;
    }

    d3d_context->CopyResource(staging_tex, tex);
    duplication->ReleaseFrame();

    D3D11_MAPPED_SUBRESOURCE map;

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg["polling_rate"]));
    }
    while (d3d_context->Map(staging_tex, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &map) == DXGI_ERROR_WAS_STILL_DRAWING);

    d3d_context->Unmap(staging_tex, 0);
    staging_tex->Release();
    d3d_context->Release();

    LOGV << "Copying buffer";
    memcpy(buf.data(), reinterpret_cast<uint8_t*>(map.pData), buf.size());

    return true;
}

void DXGIDupl::restartDXGI()
{
    HRESULT hr;

    do
    {
        hr = output1->DuplicateOutput(d3d_device, &duplication);

        IF_PLOG(plog::debug)
        {
            if(hr != S_OK)
            {
                LOGE << "** Unable to duplicate output. Reason:";

                switch (hr)
                {
                    case E_INVALIDARG:                          LOGE << "E_INVALIDARG"; break;
                    case E_ACCESSDENIED:                        LOGE << "E_ACCESSDENIED"; break;
                    case DXGI_ERROR_UNSUPPORTED:                LOGE << "E_DXGI_ERROR_UNSUPPORTED"; break;
                    case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:    LOGE << "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE"; break;
                    case DXGI_ERROR_SESSION_DISCONNECTED:       LOGE << "DXGI_ERROR_SESSION_DISCONNECTED"; break;
                }

                LOGI << "Retrying... (2.5 sec)";
            }
        }

        Sleep(2500);
    }
    while (hr != S_OK);

    LOGI << "Output duplication restarted successfully";

    output1->Release();
}

DXGIDupl::~DXGIDupl()
{
    if(duplication) duplication->ReleaseFrame();
    if(output1)     output1->Release();
    if(d3d_context) d3d_context->Release();
    if(d3d_device)  d3d_device->Release();
}
