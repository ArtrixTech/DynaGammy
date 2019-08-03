#ifdef _WIN32
#include "main.h"
#include "DXGIDupl.h"
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>

DXGIDupl::DXGIDupl() {}

bool DXGIDupl::initDXGI()
{
    #ifdef dbg
    std::cout << "Initializing DXGI...\n";
    #endif

    IDXGIOutput* output;
    IDXGIAdapter1* pAdapter;
    std::vector<IDXGIOutput*> vOutputs; //Monitors vector
    std::vector<IDXGIAdapter1*> vAdapters; //GPUs vector

    HRESULT hr;

    //Retrieve a IDXGIFactory to enumerate the adapters
    {
        IDXGIFactory1* pFactory = nullptr;
        hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory));

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Error: failed to retrieve the IDXGIFactory.\n";
            getchar();
            #endif
            return false;
        }

        UINT i = 0;

        while (pFactory->EnumAdapters1(i++, &pAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            vAdapters.push_back(pAdapter);
        }

        pFactory->Release();

        #ifdef dbg
        //Get GPU info
        DXGI_ADAPTER_DESC1 desc;

        for (UINT i = 0; i < vAdapters.size(); ++i)
        {
            pAdapter = vAdapters[i];
            hr = pAdapter->GetDesc1(&desc);

            if (hr != S_OK)
            {
                std::cout << "Error: failed to get a description for the adapter: " << i << '\n';
                continue;
            }

            std::wprintf(L"Adapter %i: %lS\n", i, desc.Description);
        }
        #endif
    }

    //Get the monitors attached to the GPUs
    {
        UINT j;

        for (UINT i = 0; i < vAdapters.size(); ++i)
        {
            j = 0;
            pAdapter = vAdapters[i];
            while (pAdapter->EnumOutputs(j++, &output) != DXGI_ERROR_NOT_FOUND)
            {
                #ifdef dbg
                std::printf("Found monitor %d on adapter %d\n", j, i);
                #endif
                vOutputs.push_back(output);
            }
        }

        if (vOutputs.empty())
        {
            #ifdef dbg
            std::printf("Error: no outputs found (%zu).\n", vOutputs.size());
            getchar();
            #endif

            return false;
        }

        #ifdef dbg
            //Print monitor info.
            DXGI_OUTPUT_DESC desc;

            for (size_t i = 0; i < vOutputs.size(); ++i)
            {
                output = vOutputs[i];
                hr = output->GetDesc(&desc);

                if (hr != S_OK)
                {
                    printf("Error: failed to retrieve a DXGI_OUTPUT_DESC for output %llu.\n", i);
                    continue;
                }

                std::wprintf(L"Monitor: %s, attached to desktop: %c\n", desc.DeviceName, (desc.AttachedToDesktop) ? 'y' : 'n');
            }
        #endif
    }

    //Create a Direct3D device to access the OutputDuplication interface
    {
        D3D_FEATURE_LEVEL d3d_feature_level;
        IDXGIAdapter1* d3d_adapter;
        UINT use_adapter = 0;

        if (vAdapters.size() <= use_adapter)
        {
            #ifdef dbg
            std::printf("Invalid adapter index: %d, we only have: %zu - 1\n", use_adapter, vAdapters.size());
            getchar();
            #endif

            return false;
        }

        d3d_adapter = vAdapters[use_adapter];

        if (!d3d_adapter)
        {
            #ifdef dbg
            std::cout << "Error: the stored adapter is nullptr.\n";
            getchar();
            #endif

            return false;
        }

        hr = D3D11CreateDevice(
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
            #ifdef dbg
            std::cout << "Error: failed to create the D3D11 Device.\n";

            if (hr == E_INVALIDARG)
            {
                std::cout << "Got INVALID arg passed into D3D11CreateDevice. Did you pass a adapter + a driver which is not the UNKNOWN driver?\n";
            }

            getchar();
            #endif

            return false;
        }
    }

    //Choose what monitor to use
    {
        UINT use_monitor = 0;
        output = vOutputs[use_monitor];

        if (vOutputs.size() <= use_monitor)
        {
            #ifdef dbg
            std::printf("Invalid monitor index: %d, we only have: %zu - 1\n", use_monitor, vOutputs.size());
            getchar();
            #endif

            return false;
        }

        if (!output)
        {
            #ifdef dbg
            std::cout << "No valid output found. The output is nullptr.\n";
            getchar();
            #endif

            return false;
        }
    }

    //Set texture properties
    {
        DXGI_OUTPUT_DESC desc;
        hr = output->GetDesc(&desc);

        bufLen = desc.DesktopCoordinates.right * desc.DesktopCoordinates.bottom * 4;

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Error: failed to get output description.\n";
            getchar();
            #endif

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

    //Initialize output duplication
    {
        hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&output1));

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Error: failed to query the IDXGIOutput1 interface.\n";
            getchar();
            #endif

            return false;
        }

        hr = output1->DuplicateOutput(d3d_device, &duplication);

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Error: DuplicateOutput failed.\n";
            getchar();
            #endif

            return false;
        }

        output1->Release();
        d3d_device->Release();
    }

    for (auto adapter : vAdapters) adapter->Release();
    for (auto output : vOutputs) output->Release();

    return true;
}

bool DXGIDupl::getDXGISnapshot(uint8_t* buf) noexcept
{
    HRESULT hr;

    ID3D11Texture2D* tex;
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    IDXGIResource* desktop_resource;

    hr = duplication->AcquireNextFrame(INFINITE, &frame_info, &desktop_resource);

    switch(hr)
    {
        case S_OK:
        {
            //Get the texture interface
            hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));

            desktop_resource->Release();
            tex->Release();

            if (hr != S_OK)
            {
                #ifdef dbg
                std::cout << "Error: failed to query the ID3D11Texture2D interface on the IDXGIResource.\n";
                #endif
                return false;
            }

            break;
        }
        case DXGI_ERROR_ACCESS_LOST:
        {
            #ifdef dbg
            std::cout << "Received a DXGI_ERROR_ACCESS_LOST.\n";
            #endif
            return false;
        }
        case DXGI_ERROR_WAIT_TIMEOUT:
        {
            #ifdef dbg
            std::cout << "Received a DXGI_ERROR_WAIT_TIMEOUT.\n";
            #endif
            return false;
        }
        default:
        {
            #ifdef dbg
            std::cout << "Error: failed to acquire frame.\n";
            #endif
            return false;
        }
     };

    ID3D11Texture2D* staging_tex;
    hr = d3d_device->CreateTexture2D(&tex_desc, nullptr, &staging_tex);

    if (hr != S_OK)
    {
        #ifdef dbg
        std::cout << "Failed to create the 2D texture, error: " << hr << '\n';
        #endif
        return false;
    }

    d3d_context->CopyResource(staging_tex, tex);
    duplication->ReleaseFrame();

    D3D11_MAPPED_SUBRESOURCE map;

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(polling_rate_ms));
    }
    while (d3d_context->Map(staging_tex, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &map) == DXGI_ERROR_WAS_STILL_DRAWING);

    d3d_context->Unmap(staging_tex, 0);
    staging_tex->Release();
    d3d_context->Release();

    #ifdef dbg
    //std::cout << "depthPitch = " << map.DepthPitch << '\n';
    #endif

    //buf = reinterpret_cast<uint8_t*>(map.pData); //750 Ti: stable, Intel HD 2000: can crash when reading the buffer in calcBrightness
    memcpy(buf, map.pData, bufLen); //750 Ti: stable, Intel HD 2000: can cause a crash when copying

    return true;
}

void DXGIDupl::restartDXGI()
{
    HRESULT hr;

    do {
        hr = output1->DuplicateOutput(d3d_device, &duplication);

        #ifdef dbg
        if (hr != S_OK)
        {
            std::cout << "Unable to duplicate output. Reason: ";

            switch (hr) {
            case E_INVALIDARG:	 printf("E_INVALIDARG\n"); break;
            case E_ACCESSDENIED: printf("E_ACCESSDENIED\n"); break;
            case DXGI_ERROR_UNSUPPORTED: printf("E_DXGI_ERROR_UNSUPPORTED\n"); break;
            case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: printf("DXGI_ERROR_NOT_CURRENTLY_AVAILABLE\n"); break;
            case DXGI_ERROR_SESSION_DISCONNECTED: printf("DXGI_ERROR_SESSION_DISCONNECTED\n"); break;
            }

            std::cout << "Retrying... (5 sec)\n";
        }
        #endif

        Sleep(5000);
    } while (hr != S_OK);

#ifdef dbg
    std::cout << "Output duplication restarted successfully.\n";
#endif

    output1->Release();
}

DXGIDupl::~DXGIDupl()
{
    if(duplication) duplication->ReleaseFrame();
    if(output1)     output1->Release();
    if(d3d_context) d3d_context->Release();
    if(d3d_device)  d3d_device->Release();
}
#endif
