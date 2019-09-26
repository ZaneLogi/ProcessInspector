#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class BitmapFontShader
{
private:
    struct ConstantBufferType
    {
        XMMATRIX projection;
    };

    struct PixelBufferType
    {
        XMFLOAT4 pixelColor;
    };

public:
    BitmapFontShader() = default;
    BitmapFontShader(const BitmapFontShader&) = delete;

    bool initialize(ID3D11Device*, HWND);
    void shutdown();
    bool render(ID3D11DeviceContext*, int, const XMMATRIX&, ID3D11ShaderResourceView*, XMFLOAT4);

private:
    bool InitializeShader(ID3D11Device*, HWND);
    void ShutdownShader();
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

    bool SetShaderParameters(ID3D11DeviceContext*, const XMMATRIX&, ID3D11ShaderResourceView*, XMFLOAT4);
    void RenderShader(ID3D11DeviceContext*, int);

private:
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_layout = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;
    ID3D11SamplerState* m_sampleState = nullptr;

    ID3D11Buffer* m_pixelBuffer = nullptr;
};

