#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class BitmapFont
{
private:
    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

public:
    BitmapFont() = default;
    BitmapFont(const BitmapFont&) = delete;

    bool initialize(ID3D11Device*, ID3D11DeviceContext*);
    void shutdown();

    ID3D11ShaderResourceView* get_texture();

    void build_vertex_array(void*, char*, float, float);

private:
    bool CreateFontTexture(ID3D11Device*, ID3D11DeviceContext* deviceContext);
    void ReleaseFontTexture();

private:
    ABC m_abc[256];
    int m_char_width;
    int m_char_height;
    int m_dib_width;
    int m_dib_height;
    ID3D11Texture2D* m_pFontTexture = nullptr;
    ID3D11ShaderResourceView* m_pFontTextureView = nullptr;
};


