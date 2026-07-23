#pragma once

#include <windows.h>
#include <d3d11.h>
#include "imgui.h"

// Only ever included from gui_main.cpp's translation unit, so defining the
// implementation here (rather than in a .cpp) doesn't risk multiple definitions.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Decodes a PNG embedded as an RCDATA resource into a D3D11 texture ImGui can draw.
class IconTexture {
public:
    ~IconTexture() {
        if (srv_)     srv_->Release();
        if (texture_) texture_->Release();
    }

    bool LoadFromResource(ID3D11Device* device, int resourceId) {
        HMODULE module = GetModuleHandleW(nullptr);
        HRSRC   res    = FindResourceW(module, MAKEINTRESOURCEW(resourceId), MAKEINTRESOURCEW(10) /* RT_RCDATA */);
        if (!res) return false;

        HGLOBAL handle = LoadResource(module, res);
        if (!handle) return false;

        void* data = LockResource(handle);
        DWORD size = SizeofResource(module, res);
        if (!data || size == 0) return false;

        int width, height, channels;
        unsigned char* pixels = stbi_load_from_memory(
            static_cast<const unsigned char*>(data), static_cast<int>(size),
            &width, &height, &channels, 4);
        if (!pixels) return false;

        bool ok = CreateTexture(device, pixels, width, height);
        stbi_image_free(pixels);
        return ok;
    }

    bool IsLoaded() const { return srv_ != nullptr; }

    ImTextureRef TexRef() const {
        return ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(srv_)));
    }

private:
    ID3D11Texture2D*          texture_ = nullptr;
    ID3D11ShaderResourceView* srv_     = nullptr;

    bool CreateTexture(ID3D11Device* device, unsigned char* pixels, int width, int height) {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width            = width;
        desc.Height           = height;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sub = {};
        sub.pSysMem     = pixels;
        sub.SysMemPitch = width * 4;

        if (FAILED(device->CreateTexture2D(&desc, &sub, &texture_))) return false;
        if (FAILED(device->CreateShaderResourceView(texture_, nullptr, &srv_))) {
            texture_->Release();
            texture_ = nullptr;
            return false;
        }
        return true;
    }
};
