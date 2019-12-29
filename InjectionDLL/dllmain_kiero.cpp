#include "stdafx.h"
#include <memory>
#include "Logger.h"
#include "kiero.h"

#include "d3d11_method_table.h"
#include "d3d10_method_table.h"
#include "d3d9_method_table.h"

#include "apihook.hpp"

#include "../minhook/include/MinHook.h"

#include "server_connection.h"

#include <d3d9.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <directxmath.h>
using namespace DirectX;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


typedef HRESULT(STDMETHODCALLTYPE *DXGISwapChainPresentType)(IDXGISwapChain *, UINT, UINT);
std::unique_ptr<ApiHook<DXGISwapChainPresentType>> g_dxgiSwapChainPresentHook;




// Create the type of function that we will hook
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
static EndScene oEndScene = NULL;

// Declare the detour function
long __stdcall hkD3D9EndScene(LPDIRECT3DDEVICE9 pDevice)
{
    // ... Your magic here ...

#if 1
    static bool init = false;
    if (!init)
    {
        //MessageBox(0, _T("Boom! It's works!"), _T("Kiero"), MB_OK);
        LOG << "D3D9: Boom! It's works!\n";
        init = true;
    }
#endif

    return oEndScene(pDevice);
}

typedef long (__stdcall* IDXGISwapChain_Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
//static IDXGISwapChain_Present originalD3D11Present = nullptr;

#define D3DHOOK_SAMPLE_INDEX 1

#if D3DHOOK_SAMPLE_INDEX == 1





static bool g_screen_size_changed = false;
static int g_screen_width = 0;
static int g_screen_height = 0;

#include "Text.h"
Text g_text;




class D3D11Core
{
public:
    D3D11Core() = default;
    D3D11Core(const D3D11Core&) = delete;

    bool Init(ID3D11Device* d3d_dev)
    {
        m_ready = InitD3DStates(d3d_dev);
        return m_ready;
    }

    void Shutdown()
    {
    }

    void DisableDepthStencilState(ID3D11DeviceContext* d3d_dev_context)
    {
        d3d_dev_context->OMGetDepthStencilState(&m_oldDepthStencilState, &m_odlStencilRef);
        d3d_dev_context->OMSetDepthStencilState(m_depthDisabledStencilState, 1);
    }

    void RestoreDepthStencilState(ID3D11DeviceContext* d3d_dev_context)
    {
        d3d_dev_context->OMSetDepthStencilState(m_oldDepthStencilState, m_odlStencilRef);
    }

    void EnableAlphaBlendState(ID3D11DeviceContext* d3d_dev_context)
    {
        d3d_dev_context->OMGetBlendState(&m_oldBlendState, m_oldBlendFactor, &m_oldSampleMask);

        float blendFactor[4];
        blendFactor[0] = 0.0f;
        blendFactor[1] = 0.0f;
        blendFactor[2] = 0.0f;
        blendFactor[3] = 0.0f;

        d3d_dev_context->OMSetBlendState(m_alphaEnableBlendingState, blendFactor, 0xffffffff);
    }

    void RestoreBlendState(ID3D11DeviceContext* d3d_dev_context)
    {
        d3d_dev_context->OMSetBlendState(m_oldBlendState, m_oldBlendFactor, m_oldSampleMask);
    }

private:
    bool InitD3DStates(ID3D11Device* d3d_dev)
    {
        HRESULT hr;
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
        depthStencilDesc.DepthEnable = false;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
        depthStencilDesc.StencilEnable = true;
        depthStencilDesc.StencilReadMask = 0xFF;
        depthStencilDesc.StencilWriteMask = 0xFF;
        depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        hr = d3d_dev->CreateDepthStencilState(&depthStencilDesc, &m_depthDisabledStencilState);
        if (FAILED(hr))
        {
            LOG << "Failed to call CreateDepthStencilState(disable), error = " << hr << "\n";
            return false;
        }

        D3D11_BLEND_DESC blendStateDescription;
        ZeroMemory(&blendStateDescription, sizeof(D3D11_BLEND_DESC));
        blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
        blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendStateDescription.RenderTarget[0].RenderTargetWriteMask = 0x0f;

        hr = d3d_dev->CreateBlendState(&blendStateDescription, &m_alphaEnableBlendingState);
        if (FAILED(hr))
        {
            return false;
        }


        return true;
    }

private:
    bool                        m_ready = false;

    ID3D11DepthStencilState*    m_oldDepthStencilState = nullptr;
    UINT                        m_odlStencilRef;
    ID3D11DepthStencilState*    m_depthDisabledStencilState = nullptr;

    ID3D11BlendState*           m_oldBlendState = nullptr;
    FLOAT                       m_oldBlendFactor[4];
    UINT                        m_oldSampleMask;
    ID3D11BlendState*           m_alphaEnableBlendingState = nullptr;
};

class RectangleModel
{
private:
    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

public:
    RectangleModel() = default;
    RectangleModel(const RectangleModel&) = delete;

    bool Init(ID3D11Device* d3d_dev)
    {
        m_ready = InitBuffers(d3d_dev);
        return m_ready;
    }

    bool Render(ID3D11DeviceContext* d3d_dev_context, int position_x, int position_y, bool screen_size_changed)
    {
        if (m_ready)
        {
            if (!UpdateBuffers(d3d_dev_context, position_x, position_y, screen_size_changed))
            {
                return false;
            }

            RenderBuffers(d3d_dev_context);
        }
        return true;
    }

    int GetIndexCount() const
    {
        return c_index_count;
    }

private:
    bool InitBuffers(ID3D11Device* d3d_dev)
    {
        VertexType vertices[c_vertex_count];
        unsigned long indices[c_index_count] = { 0, 1, 2, 3, 4, 5 };
        D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
        D3D11_SUBRESOURCE_DATA vertexData, indexData;
        HRESULT result;

        // Set up the description of the static vertex buffer.
        vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vertexBufferDesc.ByteWidth = sizeof(VertexType) * c_vertex_count;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vertexBufferDesc.MiscFlags = 0;
        vertexBufferDesc.StructureByteStride = 0;

        // Give the subresource structure a pointer to the vertex data.
        vertexData.pSysMem = vertices;
        vertexData.SysMemPitch = 0;
        vertexData.SysMemSlicePitch = 0;

        // Now create the vertex buffer.
        result = d3d_dev->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
        if (FAILED(result))
        {
            LOG << "Failed to call CreateBuffer(vertex), error = " << result << "\n";
            return false;
        }

        // Set up the description of the static index buffer.
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = sizeof(unsigned long) * c_index_count;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;
        indexBufferDesc.StructureByteStride = 0;

        // Give the subresource structure a pointer to the index data.
        indexData.pSysMem = indices;
        indexData.SysMemPitch = 0;
        indexData.SysMemSlicePitch = 0;

        // Create the index buffer.
        result = d3d_dev->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
        if (FAILED(result))
        {
            LOG << "Failed to call CreateBuffer(index), error = " << result << "\n";
            return false;
        }

        LOG << "Succeeded to init_d3d_buffers\n";
        return true;
    }

    bool UpdateBuffers(ID3D11DeviceContext* d3d_dev_context, int position_x, int position_y, bool screen_size_changed)
    {
        float left, right, top, bottom;
        VertexType vertices[c_vertex_count];
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        VertexType* verticesPtr;
        HRESULT result;

        // If the position we are rendering this bitmap to has not changed then don't update the vertex buffer since it
        // currently has the correct parameters.
        if ((position_x == m_previous_x) && (position_y == m_previous_y) && !screen_size_changed)
        {
            return true;
        }

        LOG << "need to update buffers, x = " << position_x << ", y = " << position_y << "\n";

        // If it has changed then update the position it is being rendered to.
        m_previous_x = position_x;
        m_previous_y = position_y;

        // Calculate the screen coordinates of the left side of the bitmap.
        left = (float)((g_screen_width / 2) * -1) + (float)position_x;

        // Calculate the screen coordinates of the right side of the bitmap.
        right = left + (float)m_rect_width;

        // Calculate the screen coordinates of the top of the bitmap.
        top = (float)(g_screen_height / 2) - (float)position_y;

        // Calculate the screen coordinates of the bottom of the bitmap.
        bottom = top - (float)m_rect_height;

        const float Z = 1.0f;

        // Load the vertex array with data.
        // First triangle.
        vertices[0].position = XMFLOAT3(left, top, Z);  // Top left.
        vertices[0].color = XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);

        vertices[1].position = XMFLOAT3(right, bottom, Z);  // Bottom right.
        vertices[1].color = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

        vertices[2].position = XMFLOAT3(left, bottom, Z);  // Bottom left.
        vertices[2].color = XMFLOAT4(0.0f, 0.0f, 0.5f, 1.0f);

        // Second triangle.
        vertices[3].position = XMFLOAT3(left, top, Z);  // Top left.
        vertices[3].color = XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);

        vertices[4].position = XMFLOAT3(right, top, Z);  // Top right.
        vertices[4].color = XMFLOAT4(0.0f, 0.0f, 0.5f, 1.0f);

        vertices[5].position = XMFLOAT3(right, bottom, Z);  // Bottom right.
        vertices[5].color = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

        // Lock the vertex buffer so it can be written to.
        result = d3d_dev_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(result))
        {
            LOG << "Failed to call Map(vertexBuffer), error = " << result << "\n";
            return false;
        }

        // Get a pointer to the data in the vertex buffer.
        verticesPtr = (VertexType*)mappedResource.pData;

        // Copy the data into the vertex buffer.
        memcpy(verticesPtr, (void*)vertices, (sizeof(VertexType) * c_vertex_count));

        // Unlock the vertex buffer.
        d3d_dev_context->Unmap(m_vertexBuffer, 0);

        return true;
    }

    bool RenderBuffers(ID3D11DeviceContext* d3d_dev_context)
    {
        unsigned int stride;
        unsigned int offset;

        // Set vertex buffer stride and offset.
        stride = sizeof(VertexType);
        offset = 0;

        // Set the vertex buffer to active in the input assembler so it can be rendered.
        d3d_dev_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

        // Set the index buffer to active in the input assembler so it can be rendered.
        d3d_dev_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
        d3d_dev_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        return true;
    }

private:
    bool m_ready = false;

    static const int c_vertex_count = 6;
    static const int c_index_count = 6;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;

    const int m_rect_width = 256;
    const int m_rect_height = 64;

    int m_previous_x = -1;
    int m_previous_y = -1;
};


const char RectangleShaderText[] = R"(
cbuffer MatrixBuffer
{
    matrix orthoMatrix;
};
struct VertexInputType
{
    float4 position : POSITION;
    float4 color : COLOR;
};
struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};
PixelInputType ColorVertexShader(VertexInputType input)
{
    PixelInputType output;
    input.position.w = 1.0f;
    output.position = mul(input.position, orthoMatrix);
    output.color = input.color;
    return output;
}
float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    return input.color;
}
)";

const int RectangleShaderTextLength = sizeof(RectangleShaderText);

class RectangleShader
{
private:
    struct MatrixBufferType
    {
        XMMATRIX ortho;
    };

public:
    RectangleShader() = default;
    RectangleShader(const RectangleShader&) = delete;

    bool Init(ID3D11Device* d3d_dev)
    {
        m_ready = InitShader(d3d_dev);
        return m_ready;
    }

    bool Render(ID3D11DeviceContext* d3d_dev_context, int indexCount, const XMMATRIX& orthoMatrix)
    {
        if (m_ready)
        {
            if (!SetShaderParameters(d3d_dev_context, orthoMatrix))
            {
                return false;
            }

            RenderShader(d3d_dev_context, indexCount);
        }
        return true;
    }

private:
    bool InitShader(ID3D11Device* d3d_dev)
    {
        HRESULT result;
        ID3D10Blob* errorMessage = nullptr;
        ID3D10Blob* vertexShaderBuffer = nullptr;
        ID3D10Blob* pixelShaderBuffer = nullptr;
        D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
        unsigned int numElements;
        D3D11_BUFFER_DESC matrixBufferDesc;

        // Compile the vertex shader code.
        result = D3DCompile(RectangleShaderText, RectangleShaderTextLength,
            NULL, NULL, NULL,
            "ColorVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
            &vertexShaderBuffer, &errorMessage);
        if (FAILED(result))
        {
            // If the shader failed to compile it should have writen something to the error message.
            if (errorMessage)
            {
                OutputShaderErrorMessage(errorMessage);
            }
            // If there was  nothing in the error message then it simply could not find the shader file itself.
            else
            {
                LOG << "Failed to call D3DCompile(VertexShaderText), error = " << result << "\n";
            }
            return false;
        }

        // Compile the pixel shader code.
        result = D3DCompile(RectangleShaderText, RectangleShaderTextLength,
            NULL, NULL, NULL, "ColorPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
            &pixelShaderBuffer, &errorMessage);
        if (FAILED(result))
        {
            // If the shader failed to compile it should have writen something to the error message.
            if (errorMessage)
            {
                OutputShaderErrorMessage(errorMessage);
            }
            // If there was nothing in the error message then it simply could not find the file itself.
            else
            {
                LOG << "Failed to call D3DCompile(PixelShaderText), error = " << result << "\n";
            }
            return false;
        }

        // Create the vertex shader from the buffer.
        result = d3d_dev->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
        if (FAILED(result))
        {
            LOG << "Failed to call CreateVertexShader, error = " << result << "\n";
            return false;
        }

        // Create the pixel shader from the buffer.
        result = d3d_dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
        if (FAILED(result))
        {
            LOG << "Failed to call CreatePixelShader, error = " << result << "\n";
            return false;
        }

        // Create the vertex input layout description.
        // This setup needs to match the VertexType stucture in the ModelClass and in the shader.
        polygonLayout[0].SemanticName = "POSITION";
        polygonLayout[0].SemanticIndex = 0;
        polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        polygonLayout[0].InputSlot = 0;
        polygonLayout[0].AlignedByteOffset = 0;
        polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        polygonLayout[0].InstanceDataStepRate = 0;

        polygonLayout[1].SemanticName = "COLOR";
        polygonLayout[1].SemanticIndex = 0;
        polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        polygonLayout[1].InputSlot = 0;
        polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        polygonLayout[1].InstanceDataStepRate = 0;

        // Get a count of the elements in the layout.
        numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

        // Create the vertex input layout.
        result = d3d_dev->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
            vertexShaderBuffer->GetBufferSize(), &m_layout);
        if (FAILED(result))
        {
            LOG << "Failed to call CreateInputLayout, error = " << result << "\n";
            return false;
        }

        // Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
        vertexShaderBuffer->Release();
        vertexShaderBuffer = 0;

        pixelShaderBuffer->Release();
        pixelShaderBuffer = 0;

        // Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
        matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
        matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        matrixBufferDesc.MiscFlags = 0;
        matrixBufferDesc.StructureByteStride = 0;

        // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
        result = d3d_dev->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
        if (FAILED(result))
        {
            LOG << "Failed to call CreateBuffer(matrix), error = " << result << "\n";
            return false;
        }

        LOG << "Succeeded to init_d3d_shaders\n";
        return true;
    }

    void OutputShaderErrorMessage(ID3D10Blob* errorMessage)
    {
        char* compileErrors;
        unsigned long long bufferSize, i;

        // Get a pointer to the error message text buffer.
        compileErrors = (char*)(errorMessage->GetBufferPointer());

        // Get the length of the message.
        bufferSize = errorMessage->GetBufferSize();

        // Write out the error message.
        for (i = 0; i<bufferSize; i++)
        {
            LOG << compileErrors[i];
        }

        // Release the error message.
        errorMessage->Release();
        errorMessage = nullptr;
    }

    bool SetShaderParameters(
        ID3D11DeviceContext* d3d_dev_context,
        const XMMATRIX& orthoMatrix)
    {
        HRESULT result;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        MatrixBufferType* dataPtr;
        unsigned int bufferNumber;

        // Lock the constant buffer so it can be written to.
        result = d3d_dev_context->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(result))
        {
            LOG << "Failed to call Map(matrixBuffer), error = " << result << "\n";
            return false;
        }

        // Get a pointer to the data in the constant buffer.
        dataPtr = (MatrixBufferType*)mappedResource.pData;

        // Copy the matrices into the constant buffer.
        dataPtr->ortho = XMMatrixTranspose(orthoMatrix);;

        // Unlock the constant buffer.
        d3d_dev_context->Unmap(m_matrixBuffer, 0);

        // Set the position of the constant buffer in the vertex shader.
        bufferNumber = 0;

        // Finanly set the constant buffer in the vertex shader with the updated values.
        d3d_dev_context->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);

        return true;
    }

    void RenderShader(ID3D11DeviceContext* d3d_dev_context, int indexCount)
    {
        // Set the vertex input layout.
        d3d_dev_context->IASetInputLayout(m_layout);

        // Set the vertex and pixel shaders that will be used to render this triangle.
        d3d_dev_context->VSSetShader(m_vertexShader, NULL, 0);
        d3d_dev_context->PSSetShader(m_pixelShader, NULL, 0);

        // Render the triangle.
        d3d_dev_context->DrawIndexed(indexCount, 0, 0);
    }

private:
    bool                     m_ready = false;
    ID3D11VertexShader*      m_vertexShader = nullptr;
    ID3D11PixelShader*       m_pixelShader = nullptr;
    ID3D11InputLayout*       m_layout = nullptr;
    ID3D11Buffer*            m_matrixBuffer = nullptr;
};


static D3D11Core g_d3d11core;
static RectangleModel g_model;
static RectangleShader g_shader;




bool init_d3d_resources(ID3D11Device* d3d_device, ID3D11DeviceContext* d3d_dev_context)
{
    if (!g_d3d11core.Init(d3d_device))
    {
        LOG << "g_d3d11core.Init failed!\n";
        return false;
    }

    if (!g_model.Init(d3d_device))
    {
        LOG << "g_model.Init failed!\n";
        return false;
    }

    if (!g_shader.Init(d3d_device))
    {
        LOG << "g_model.Init failed!\n";
        return false;
    }

    if (!g_text.initialize(d3d_device, d3d_dev_context, NULL, g_screen_width, g_screen_height))
    {
        LOG << "g_text.initialize failed!\n";
        return false;
    }

    return true;
}

bool render(ID3D11DeviceContext* device_context)
{
    g_d3d11core.DisableDepthStencilState(device_context);

    auto ortho_matrix = XMMatrixOrthographicLH((float)g_screen_width, (float)g_screen_height, 1.0f, 1000.0f);

    g_model.Render(device_context, 0, 0, g_screen_size_changed);

    g_shader.Render(device_context, g_model.GetIndexCount(), ortho_matrix);

    g_d3d11core.EnableAlphaBlendState(device_context);
    g_text.render(device_context, ortho_matrix);
    g_d3d11core.RestoreBlendState(device_context);

    g_d3d11core.RestoreDepthStencilState(device_context);
    return true;
}

//
//
//
long __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    static bool init = false;
    static BOOL windowed = true;

    static ID3D11Device* pPreviousDevice = nullptr;

    HRESULT hr = S_OK;
    DXGI_SWAP_CHAIN_DESC dscd;
    hr = pSwapChain->GetDesc(&dscd);

    bool is_d3d11dev = false;
    bool is_d3d10dev = false;

    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pDeviceContext = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (LPVOID*)&pDevice);
    }
    if (SUCCEEDED(hr))
    {
        is_d3d11dev = true;
        pDevice->GetImmediateContext(&pDeviceContext);
    }

    ID3D10Device* pd3d10Device = nullptr;
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(pd3d10Device), (LPVOID*)&pd3d10Device)))
    {
        is_d3d10dev = true;
        pd3d10Device->Release();
    }

    if (FAILED(hr) || !pDevice || !pDeviceContext)
    {
        static bool err_once = false;
        if (!err_once)
        {
            err_once = true;
            LOG << "direct call original function Present!\n";
        }
        //return originalD3D11Present(pSwapChain, SyncInterval, Flags);
        return g_dxgiSwapChainPresentHook->callOrginal<HRESULT>(pSwapChain, SyncInterval, Flags);
    }

    if (!init)
    {
        init = true;
        LOG << "Boom! It's works!\n";

        bool isDX11 = (is_d3d11dev && !is_d3d10dev);
        if (isDX11)
        {
            LOG << "DX11 application!\n";
        }
        else
        {
            LOG << "DX10 application!\n";
        }

        if (SUCCEEDED(hr))
        {
            LOG << "BufferCount = " << dscd.BufferCount <<
                ", Width = " << dscd.BufferDesc.Width <<
                ", Height = " << dscd.BufferDesc.Height <<
                ", Windowed = " << dscd.Windowed << "\n";

            g_screen_width = dscd.BufferDesc.Width;
            g_screen_height = dscd.BufferDesc.Height;
            g_screen_size_changed = true;
            windowed = dscd.Windowed;

            init_d3d_resources(pDevice, pDeviceContext);

            pPreviousDevice = pDevice;

        }
    }
    else
    {
        if (SUCCEEDED(hr) && (windowed != dscd.Windowed || g_screen_width != dscd.BufferDesc.Width || g_screen_height != dscd.BufferDesc.Height))
        {
            LOG << "BufferCount = " << dscd.BufferCount <<
                ", Width = " << dscd.BufferDesc.Width <<
                ", Height = " << dscd.BufferDesc.Height <<
                ", Windowed = " << dscd.Windowed << "\n";

            g_screen_width = dscd.BufferDesc.Width;
            g_screen_height = dscd.BufferDesc.Height;
            windowed = dscd.Windowed;
            g_screen_size_changed = true;
        }
        else
        {
            g_screen_size_changed = false;
        }
    }

    if (pPreviousDevice != pDevice)
    {
        LOG << "device pointer is not same!!!\n";
    }

    if (SUCCEEDED(hr))
    {
        render(pDeviceContext);
    }

    if (pDeviceContext)
    {
        pDeviceContext->Release();
        pDeviceContext = nullptr;
    }

    if (pDevice)
    {
        pDevice->Release();
        pDevice = nullptr;
    }

    //return originalD3D11Present(pSwapChain, SyncInterval, Flags);
    //return S_OK;
    return g_dxgiSwapChainPresentHook->callOrginal<HRESULT>(pSwapChain, SyncInterval, Flags);
}

#endif

#if D3DHOOK_SAMPLE_INDEX == 2
#include "shadez.h"

#define safe_release(p) if (p) { p->Release(); p = nullptr; }

ID3D11Device* pDevice = nullptr;
IDXGISwapChain* pSwapchain = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* pRenderTargetView = nullptr;
ID3D11VertexShader* pVertexShader = nullptr;
ID3D11InputLayout* pVertexLayout = nullptr;
ID3D11PixelShader* pPixelShader = nullptr;
ID3D11Buffer* pVertexBuffer = nullptr;
ID3D11Buffer* pIndexBuffer = nullptr;
ID3D11Buffer* pConstantBuffer = nullptr;

// Changing this to an array of viewports
#define MAINVP 0
D3D11_VIEWPORT pViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{ 0 };
XMMATRIX mOrtho;

struct ConstantBuffer
{
    XMMATRIX mProjection;
};

struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT4 color;
};

bool CompileShader(const char* szShader, const char * szEntrypoint, const char * szTarget, ID3D10Blob ** pBlob)
{
    ID3D10Blob* pErrorBlob = nullptr;

    auto hr = D3DCompile(szShader, strlen(szShader), 0, nullptr, nullptr, szEntrypoint, szTarget, D3DCOMPILE_ENABLE_STRICTNESS, 0, pBlob, &pErrorBlob);
    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            char szError[256]{ 0 };
            memcpy(szError, pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
            MessageBoxA(nullptr, szError, "Error", MB_OK);
        }
        return false;
    }
    return true;
}

bool InitD3DHook(IDXGISwapChain * pSwapchain)
{
    HRESULT hr = pSwapchain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);
    if (FAILED(hr))
        return false;

    pDevice->GetImmediateContext(&pContext);
    pContext->OMGetRenderTargets(1, &pRenderTargetView, nullptr);

    // If for some reason we fail to get a render target, create one.
    // This will probably never happen with a real game but maybe certain test environments... :P
    if (!pRenderTargetView)
    {
        // Get a pointer to the back buffer for the render target view
        ID3D11Texture2D* pBackbuffer = nullptr;
        hr = pSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackbuffer));
        if (FAILED(hr))
            return false;

        // Create render target view
        hr = pDevice->CreateRenderTargetView(pBackbuffer, nullptr, &pRenderTargetView);
        pBackbuffer->Release();
        if (FAILED(hr))
            return false;

        // Make sure our render target is set.
        pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
    }

    // initialize shaders
    ID3D10Blob* pBlob = nullptr;

    // create vertex shader
    if (!CompileShader(szShadez, "VS", "vs_5_0", &pBlob))
        return false;

    hr = pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader);
    if (FAILED(hr))
        return false;

    // Define/create the input layout for the vertex shader
    D3D11_INPUT_ELEMENT_DESC layout[2] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(layout);

    hr = pDevice->CreateInputLayout(layout, numElements, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &pVertexLayout);
    if (FAILED(hr))
        return false;

    safe_release(pBlob);

    // create pixel shader
    if (!CompileShader(szShadez, "PS", "ps_5_0", &pBlob))
        return false;

    hr = pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader);
    if (FAILED(hr))
        return false;

    UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    float fWidth = 0;
    float fHeight = 0;

    // Apparently this isn't universal. Works on some games
    pContext->RSGetViewports(&numViewports, pViewports);

    //
    if (!numViewports || !pViewports[MAINVP].Width)
    {
        // This should be retrieved dynamically
        //HWND hWnd0 = FindWindowA( "W2ViewportClass", nullptr );
        //HWND hWnd = FindMainWindow(GetCurrentProcessId());
        RECT rc{ 0, 0, 800, 600 };
        //if (!GetClientRect(hWnd, &rc))
        //    return false;

        //fWidth = 1600.0f;
        //fHeight = 900.0f;
        fWidth = (float)rc.right;
        fHeight = (float)rc.bottom;

        // Setup viewport
        pViewports[MAINVP].Width = (float)fWidth;
        pViewports[MAINVP].Height = (float)fHeight;
        pViewports[MAINVP].MinDepth = 0.0f;
        pViewports[MAINVP].MaxDepth = 1.0f;

        // Set viewport to context
        pContext->RSSetViewports(1, pViewports);
    }
    else
    {
        fWidth = (float)pViewports[MAINVP].Width;
        fHeight = (float)pViewports[MAINVP].Height;
    }
    // Create the constant buffer
    D3D11_BUFFER_DESC bd{ 0 };
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.Usage = D3D11_USAGE_DEFAULT;

    // Setup orthographic projection
    mOrtho = XMMatrixOrthographicLH(fWidth, fHeight, 0.0f, 1.0f);
    ConstantBuffer cb;
    cb.mProjection = mOrtho;

    D3D11_SUBRESOURCE_DATA sr{ 0 };
    sr.pSysMem = &cb;
    hr = pDevice->CreateBuffer(&bd, &sr, &pConstantBuffer);
    if (FAILED(hr))
        return false;

    // Create a triangle to render
    // Create a vertex buffer, start by setting up a description.
    ZeroMemory(&bd, sizeof(bd));
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = 3 * sizeof(Vertex);
    bd.StructureByteStride = sizeof(Vertex);

    // left and top edge of window
    float left = fWidth / -2;
    float top = fHeight / 2;

    // Width and height of triangle
    float w = 50;
    float h = 50;

    // Center position of triangle, this should center it in the screen.
    float fPosX = -1 * left;
    float fPosY = top;

    // Setup vertices of triangle
    Vertex pVerts[3] = {
        { XMFLOAT3(left + fPosX,			top - fPosY + h / 2,	1.0f),	XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(left + fPosX + w / 2,	top - fPosY - h / 2,	1.0f),	XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(left + fPosX - w / 2,	top - fPosY - h / 2,	1.0f),	XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
    };

    // create the buffer.
    ZeroMemory(&sr, sizeof(sr));
    sr.pSysMem = &pVerts;
    hr = pDevice->CreateBuffer(&bd, &sr, &pVertexBuffer);
    if (FAILED(hr))
        return false;

    // Create an index buffer
    ZeroMemory(&bd, sizeof(bd));
    ZeroMemory(&sr, sizeof(sr));

    UINT pIndices[3] = { 0, 1, 2 };
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(UINT) * 3;
    bd.StructureByteStride = sizeof(UINT);

    sr.pSysMem = &pIndices;
    hr = pDevice->CreateBuffer(&bd, &sr, &pIndexBuffer);
    if (FAILED(hr))
        return false;

    return true;
}

void CleanupD3D()
{
    safe_release(pVertexBuffer);
    safe_release(pIndexBuffer);
    safe_release(pConstantBuffer);
    safe_release(pPixelShader);
    safe_release(pVertexShader);
    safe_release(pVertexLayout);
}

void Render()
{
    // Make sure our render target is set.
    pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

    // Update view
    ConstantBuffer cb;
    cb.mProjection = XMMatrixTranspose(mOrtho);
    pContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
    pContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);

    // Make sure the input assembler knows how to process our verts/indices
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
    pContext->IASetInputLayout(pVertexLayout);
    pContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set the shaders we need to render our triangle
    pContext->VSSetShader(pVertexShader, nullptr, 0);
    pContext->PSSetShader(pPixelShader, nullptr, 0);

    // Set viewport to context
    pContext->RSSetViewports(1, pViewports);

    // Draw our triangle
    pContext->DrawIndexed(3, 0, 0);
}

long __stdcall hookD3D11Present(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags)
{
    static bool init = false;

    if (!init)
    {
        init = true;
        LOG << "Boom! It's works!\n";
    }

    pSwapchain = pThis;

    if (!pDevice)
    {
        if (!InitD3DHook(pThis))
            return false;
    }

    Render();

    return originalD3D11Present(pThis, SyncInterval, Flags);
}
#endif



DWORD WINAPI HookD3D(LPVOID lpThreadParameter)
{
    /*{
        wchar_t* test_string = L"Enter hook thread.\r\n";
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), test_string, (DWORD)wcslen(test_string), nullptr, nullptr);
    }*/

    LOG << "enter HookD3D\n";
    bool d3d9 = false;
    bool d3d10 = false;
    bool d3d11 = false;
    bool d3d12 = false;
    bool opengl = false;
    bool vulcan = false;

    // check 3D APIs
    if (::GetModuleHandle(L"d3d9.dll") != NULL)
    {
        LOG << "found d3d9.dll\n";
        d3d9 = true;
    }
    if (::GetModuleHandle(L"d3d10.dll") != NULL)
    {
        LOG << "found d3d10.dll\n";
        d3d10 = true;
    }
    if (::GetModuleHandle(L"d3d11.dll") != NULL)
    {
        LOG << "found d3d11.dll\n";
        d3d11 = true;
    }
    if (::GetModuleHandle(L"d3d12.dll") != NULL)
    {
        LOG << "found d3d12.dll\n";
        d3d12 = true;
    }
    if (::GetModuleHandle(L"opengl32.dll") != NULL)
    {
        LOG << "found opengl32.dll\n";
        opengl = true;
    }
    if (::GetModuleHandle(L"vulcan-1.dll") != NULL)
    {
        LOG << "found vulcan-1.dll\n";
        vulcan = true;
    }

    // try to hook dxgi
    if (d3d11_method_table::instance()->init())
    {
        LOG << "d3d11_method_table enabled\n";
        auto target = (*d3d11_method_table::instance())[8];
        g_dxgiSwapChainPresentHook.reset(new ApiHook<DXGISwapChainPresentType>(L"DXGISwapChainPresentType",
            target, (DWORD_PTR *)hookD3D11Present));
        g_dxgiSwapChainPresentHook->activeHook();


        //if (!d3d11_method_table::instance()->bind(8, (void**)&originalD3D11Present, hookD3D11Present))
        //{
        //   LOG << "Failed to call d3d11hook::bind\n";
        //}
    }
    else if (d3d10_method_table::instance()->init())
    {
        LOG << "d3d10_method_table enabled\n";
        auto target = (*d3d11_method_table::instance())[8];
        g_dxgiSwapChainPresentHook.reset(new ApiHook<DXGISwapChainPresentType>(L"DXGISwapChainPresentType",
            target, (DWORD_PTR *)hookD3D11Present));
        g_dxgiSwapChainPresentHook->activeHook();
    }
    else
    {
        LOG << "not d3d11 or d3d10\n";
    }

    if (d3d9_method_table::instance()->init())
    {
        LOG << "d3d9hook enabled\n";
        if (!d3d9_method_table::instance()->bind(42, (void**)&oEndScene, hkD3D9EndScene))
        {
            LOG << "Failed to call d3d9hook::bind\n";
        }
    }
    else
    {
        LOG << "Failed to call d3d9hook::init\n";
    }
    /*
    auto result = kiero::init(kiero::RenderType::D3D11);
    if ( result == kiero::Status::Success)
    {
        auto render_type = kiero::getRenderType();
        if (render_type == kiero::RenderType::D3D11)
        {
            LOG << "type = d3d11\n";
            // the index of the required function can be found in the METHODSTABLE.txt
            //if (kiero::bind(42, (void**)&oEndScene, hkD3D9EndScene) != kiero::Status::Success)
            //{
            //    LOG << "Failed to call kiero::bind\n";
            //}
            if (kiero::bind(8, (void**)&originalD3D11Present, hookD3D11Present) != kiero::Status::Success)
            {
                LOG << "Failed to call kiero::bind\n";
            }
        }
        else
        {
            LOG << "No interested D3D found!\n";
        }
    }
    else
    {
        LOG << "Failed to call kiero::init, err = " << result << "\n";
    }
    */

    LOG << "exit HookD3D\n";

    return 0;
}

#include <mutex>
#include <thread>

class dll_hook_thread
{
public:
    static dll_hook_thread* instance();
    ~dll_hook_thread();

    void start();
    void stop();

private:
    dll_hook_thread();
    dll_hook_thread(const dll_hook_thread&) = delete;

    static DWORD WINAPI _thread_routine(PVOID);
    void _event_loop(void);

private:
    std::mutex m_mutex;
    HANDLE m_stop_event;
    HANDLE m_thread_handle = nullptr;
};

dll_hook_thread* dll_hook_thread::instance()
{
    static dll_hook_thread this_instance;
    return &this_instance;
}

dll_hook_thread::dll_hook_thread()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

dll_hook_thread::~dll_hook_thread()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    CloseHandle(m_stop_event);
}

void dll_hook_thread::start()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    LOG << "START dll hook thread.\n";
    m_thread_handle = CreateThread(nullptr, 0, _thread_routine, this, 0, nullptr);
}

void dll_hook_thread::stop()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    LOG << "STOP dll hook thread.\n";
    // signal the thread to stop, and then wait
    SetEvent(m_stop_event);
    if (m_thread_handle != nullptr)
    {
        LOG << "waiting the hook thread.\n";
        WaitForSingleObject(m_thread_handle, INFINITE);
        CloseHandle(m_thread_handle);
        m_thread_handle = nullptr;
    }
}

DWORD dll_hook_thread::_thread_routine(PVOID pv)
{
    dll_hook_thread* pThis = (dll_hook_thread*)pv;
    pThis->_event_loop();
    return 0;
}

void dll_hook_thread::_event_loop(void)
{
    LOG << ">>> Enter _event_loop\n";

    MH_Initialize();
    HookD3D(0);

    HANDLE events[] = { m_stop_event };

    bool active = true;
    while (active)
    {
        switch (MsgWaitForMultipleObjects(_countof(events), events, FALSE, INFINITE, QS_ALLINPUT))
        {
        case WAIT_OBJECT_0:
            // stop event
            active = false;
            LOG << "stop the thread loop 1.\n";
            break;

        case WAIT_OBJECT_0 + _countof(events):
            // other event
            MSG message;
            while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                {
                    PostQuitMessage(static_cast<int>(message.wParam));
                    active = false;
                    LOG << "stop the thread loop 2.\n";
                    break;
                }
                TranslateMessage(&message);
                DispatchMessage(&message);
            }

        default:
            break;
        }
    }

    if (g_dxgiSwapChainPresentHook)
    {
        g_dxgiSwapChainPresentHook.reset(nullptr);
    }

    MH_Uninitialize();

    LOG << "<<< Exit _event_loop\n";
}


#if 1
static server_connection g_server;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
    CHAR processPath[MAX_PATH];
    CHAR modulePath[MAX_PATH];

    DisableThreadLibraryCalls(hInstance);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        GetModuleFileNameA(GetModuleHandle(NULL), processPath, MAX_PATH);
        LOG << "Inject DLL to " << processPath << "\n";

        GetModuleFileNameA(hInstance, modulePath, MAX_PATH);
        LOG << "DLL path: " << modulePath << "\n";

        LoadLibraryA(modulePath); // call this function so the dll would be kept in the injected process when the injection helper quits.


        //CreateThread(NULL, 0, HookThread, NULL, 0, NULL);

        dll_hook_thread::instance()->start();

        /*AllocConsole();
        {
            wchar_t* test_string = L"injection dll console.\r\n";
            WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), test_string, (DWORD)wcslen(test_string), nullptr, nullptr);
        }*/

        if (!g_server.connectToPipe())
        {
            LOG << "Failed to connect to the server\n";
        }
        else if (!g_server.startReading())
        {
            LOG << "Failed to start reading from the server\n";
        }
        else
        {
            LOG << "Succeeded to connect to the server\n";
            cmd_init_packet packet;
            packet.hdr.packet_type = COMMAND_INIT;
            packet.hdr.payload_size = sizeof(packet.process_id);
            packet.process_id = GetCurrentProcessId();
            g_server.write(&packet, sizeof(packet.hdr) + packet.hdr.payload_size);
        }
        break;

    case DLL_PROCESS_DETACH:
        GetModuleFileNameA(GetModuleHandle(NULL), processPath, MAX_PATH);
        LOG << "Unload DLL from " << processPath << "\n";
        if (oEndScene != nullptr)
        {
            //kiero::unbind(42);
            d3d9_method_table::instance()->unbind(42);
            oEndScene = nullptr;
        }
        //if (originalD3D11Present != nullptr)
        //{
            //kiero::unbind(8);
        //    d3d11_method_table::instance()->unbind(8);
        //    originalD3D11Present = nullptr;
        //}

        dll_hook_thread::instance()->stop();

        {
            cmd_bye_packet packet;
            packet.hdr.packet_type = COMMAND_BYE;
            packet.hdr.payload_size = sizeof(packet.process_id);
            packet.process_id = GetCurrentProcessId();
            g_server.write(&packet, sizeof(packet.hdr) + packet.hdr.payload_size);

            g_server.shutdownAndCleanUp();
        }

        //FreeConsole();
        break;
    }

    return TRUE;
}

#endif
