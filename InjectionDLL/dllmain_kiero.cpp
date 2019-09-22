#include "stdafx.h"
#include "Logger.h"
#include "kiero.h"
#include <d3d9.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <directxmath.h>
using namespace DirectX;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

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
        LOG << "Boom! It's works!\n";
        init = true;
    }
#endif

    return oEndScene(pDevice);
}

typedef long (__stdcall* IDXGISwapChain_Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static IDXGISwapChain_Present originalD3D11Present = nullptr;

#define D3DHOOK_SAMPLE_INDEX 2

#if D3DHOOK_SAMPLE_INDEX == 1

struct VertexType
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct MatrixBufferType
{
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
};

static int g_screen_width = 0;
static int g_screen_height = 0;
static const int g_rect_width = 256;
static const int g_rect_height = 64;
static const int g_vertex_count = 6;
static const int g_index_count = 6;
static ID3D11Buffer* g_vertexBuffer = nullptr;
static ID3D11Buffer* g_indexBuffer = nullptr;
static int g_previous_x = -1;
static int g_previous_y = -1;

static ID3D11DepthStencilState* g_depthStencilState = nullptr;
static ID3D11DepthStencilState* g_depthDisabledStencilState = nullptr;

static ID3D11VertexShader*      g_vertexShader = nullptr;
static ID3D11PixelShader*       g_pixelShader = nullptr;
static ID3D11InputLayout*       g_layout = nullptr;
static ID3D11Buffer*            g_matrixBuffer = nullptr;

bool init_d3d_states(ID3D11Device* d3d_device)
{
    HRESULT hr;
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
    depthStencilDesc.DepthEnable = true;
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

    // Create the depth stencil state.
    hr = d3d_device->CreateDepthStencilState(&depthStencilDesc, &g_depthStencilState);
    if (FAILED(hr))
    {
        LOG << "Failed to call CreateDepthStencilState(enable), error = " << hr << "\n";
        return false;
    }

    depthStencilDesc.DepthEnable = false;
    hr = d3d_device->CreateDepthStencilState(&depthStencilDesc, &g_depthDisabledStencilState);
    if (FAILED(hr))
    {
        LOG << "Failed to call CreateDepthStencilState(disable), error = " << hr << "\n";
        return false;
    }

    LOG << "Succeeded to init_d3d_states\n";
    return true;
}

bool init_d3d_buffers(ID3D11Device* d3d_device)
{
    VertexType vertices[g_vertex_count];
    unsigned long indices[g_index_count];
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;
    HRESULT result;

    // Set up the description of the static vertex buffer.
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * g_vertex_count;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the vertex data.
    vertexData.pSysMem = vertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    // Now create the vertex buffer.
    result = d3d_device->CreateBuffer(&vertexBufferDesc, &vertexData, &g_vertexBuffer);
    if (FAILED(result))
    {
        LOG << "Failed to call CreateBuffer(vertex), error = " << result << "\n";
        return false;
    }

    // Set up the description of the static index buffer.
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * g_index_count;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the index data.
    indexData.pSysMem = indices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    // Create the index buffer.
    result = d3d_device->CreateBuffer(&indexBufferDesc, &indexData, &g_indexBuffer);
    if (FAILED(result))
    {
        LOG << "Failed to call CreateBuffer(index), error = " << result << "\n";
        return false;
    }

    LOG << "Succeeded to init_d3d_buffers\n";
    return true;
}

void output_shader_error_message(ID3D10Blob* errorMessage)
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

const char VertexShaderText[] =
"\
cbuffer MatrixBuffer { matrix worldMatrix; matrix viewMatrix; matrix projectionMatrix; };\n\
struct VertexInputType { float4 position : POSITION; float4 color : COLOR; };\n\
struct PixelInputType { float4 position : SV_POSITION; float4 color : COLOR; };\n\
PixelInputType ColorVertexShader(VertexInputType input)\n\
{\n\
    PixelInputType output;\n\
\n\
    input.position.w = 1.0f;\n\
\n\
    output.position = mul(input.position, worldMatrix);\n\
    output.position = mul(output.position, viewMatrix);\n\
    output.position = mul(output.position, projectionMatrix);\n\
\n\
    output.color = input.color;\n\
\n\
    return output;\n\
}\0";

const int VertexShaderTextLength = sizeof(VertexShaderText);


const char PixelShaderText[] =
"\
struct PixelInputType { float4 position : SV_POSITION; float4 color : COLOR; };\n\
float4 ColorPixelShader(PixelInputType input) : SV_TARGET\n\
{\n\
    return input.color;\n\
}\0";

const int PixelShaderTextLength = sizeof(PixelShaderText);

bool init_d3d_shaders(ID3D11Device* d3d_device)
{
    HRESULT result;
    ID3D10Blob* errorMessage = nullptr;
    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;
    D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
    unsigned int numElements;
    D3D11_BUFFER_DESC matrixBufferDesc;

    // Compile the vertex shader code.
    result = D3DCompile(VertexShaderText, VertexShaderTextLength,
        NULL, NULL, NULL,
        "ColorVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &vertexShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        // If the shader failed to compile it should have writen something to the error message.
        if (errorMessage)
        {
            output_shader_error_message(errorMessage);
        }
        // If there was  nothing in the error message then it simply could not find the shader file itself.
        else
        {
            LOG << "Failed to call D3DCompile(VertexShaderText), error = " << result << "\n";
        }
        return false;
    }

    // Compile the pixel shader code.
    result = D3DCompile(PixelShaderText, PixelShaderTextLength,
        NULL, NULL, NULL, "ColorPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &pixelShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        // If the shader failed to compile it should have writen something to the error message.
        if (errorMessage)
        {
            output_shader_error_message(errorMessage);
        }
        // If there was nothing in the error message then it simply could not find the file itself.
        else
        {
            LOG << "Failed to call D3DCompile(PixelShaderText), error = " << result << "\n";
        }
        return false;
    }

    // Create the vertex shader from the buffer.
    result = d3d_device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &g_vertexShader);
    if (FAILED(result))
    {
        LOG << "Failed to call CreateVertexShader, error = " << result << "\n";
        return false;
    }

    // Create the pixel shader from the buffer.
    result = d3d_device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &g_pixelShader);
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
    result = d3d_device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
        vertexShaderBuffer->GetBufferSize(), &g_layout);
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
    result = d3d_device->CreateBuffer(&matrixBufferDesc, NULL, &g_matrixBuffer);
    if (FAILED(result))
    {
        LOG << "Failed to call CreateBuffer(matrix), error = " << result << "\n";
        return false;
    }

    LOG << "Succeeded to init_d3d_shaders\n";
    return true;
}

bool init_d3d_resources(ID3D11Device* d3d_device)
{
    init_d3d_states(d3d_device);
    init_d3d_buffers(d3d_device);
    init_d3d_shaders(d3d_device);
    return true;
}

bool update_buffers(ID3D11DeviceContext* device_context, int position_x, int position_y)
{
    float left, right, top, bottom;
    VertexType vertices[g_vertex_count];
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    VertexType* verticesPtr;
    HRESULT result;

    // If the position we are rendering this bitmap to has not changed then don't update the vertex buffer since it
    // currently has the correct parameters.
    if ((position_x == g_previous_x) && (position_y == g_previous_y))
    {
        return true;
    }

    LOG << "need to update buffers, x = " << position_x << ", y = " << position_y << "\n";

    // If it has changed then update the position it is being rendered to.
    g_previous_x = position_x;
    g_previous_y = position_y;

    // Calculate the screen coordinates of the left side of the bitmap.
    left = (float)((g_screen_width / 2) * -1) + (float)position_x;

    // Calculate the screen coordinates of the right side of the bitmap.
    right = left + (float)g_rect_width;

    // Calculate the screen coordinates of the top of the bitmap.
    top = (float)(g_screen_height / 2) - (float)position_y;

    // Calculate the screen coordinates of the bottom of the bitmap.
    bottom = top - (float)g_rect_height;

    const float Z = 100;

    // Load the vertex array with data.
    // First triangle.
    vertices[0].position = XMFLOAT3(left, top, Z);  // Top left.
    vertices[0].color = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

    vertices[1].position = XMFLOAT3(right, bottom, Z);  // Bottom right.
    vertices[1].color = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

    vertices[2].position = XMFLOAT3(left, bottom, Z);  // Bottom left.
    vertices[2].color = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

    // Second triangle.
    vertices[3].position = XMFLOAT3(left, top, Z);  // Top left.
    vertices[3].color = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

    vertices[4].position = XMFLOAT3(right, top, Z);  // Top right.
    vertices[4].color = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

    vertices[5].position = XMFLOAT3(right, bottom, Z);  // Bottom right.
    vertices[5].color = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

    // Lock the vertex buffer so it can be written to.
    result = device_context->Map(g_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        LOG << "Failed to call Map(vertexBuffer), error = " << result << "\n";
        return false;
    }

    // Get a pointer to the data in the vertex buffer.
    verticesPtr = (VertexType*)mappedResource.pData;

    // Copy the data into the vertex buffer.
    memcpy(verticesPtr, (void*)vertices, (sizeof(VertexType) * g_vertex_count));

    // Unlock the vertex buffer.
    device_context->Unmap(g_vertexBuffer, 0);

    return true;
}

bool render_buffers(ID3D11DeviceContext* device_context)
{
    unsigned int stride;
    unsigned int offset;

    // Set vertex buffer stride and offset.
    stride = sizeof(VertexType);
    offset = 0;

    // Set the vertex buffer to active in the input assembler so it can be rendered.
    device_context->IASetVertexBuffers(0, 1, &g_vertexBuffer, &stride, &offset);

    // Set the index buffer to active in the input assembler so it can be rendered.
    device_context->IASetIndexBuffer(g_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return true;
}

bool set_shader_parameters(
    ID3D11DeviceContext* device_context,
    const XMMATRIX& worldMatrix,
    const XMMATRIX& viewMatrix,
    const XMMATRIX& projectionMatrix)
{
    HRESULT result;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    MatrixBufferType* dataPtr;
    unsigned int bufferNumber;

    // Transpose the matrices to prepare them for the shader.
    auto worldMatrix1 = XMMatrixTranspose(worldMatrix);
    auto viewMatrix1 = XMMatrixTranspose(viewMatrix);
    auto projectionMatrix1 = XMMatrixTranspose(projectionMatrix);

    // Lock the constant buffer so it can be written to.
    result = device_context->Map(g_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        LOG << "Failed to call Map(matrixBuffer), error = " << result << "\n";
        return false;
    }

    // Get a pointer to the data in the constant buffer.
    dataPtr = (MatrixBufferType*)mappedResource.pData;

    // Copy the matrices into the constant buffer.
    dataPtr->world = worldMatrix1;
    dataPtr->view = viewMatrix1;
    dataPtr->projection = projectionMatrix1;

    // Unlock the constant buffer.
    device_context->Unmap(g_matrixBuffer, 0);

    // Set the position of the constant buffer in the vertex shader.
    bufferNumber = 0;

    // Finanly set the constant buffer in the vertex shader with the updated values.
    device_context->VSSetConstantBuffers(bufferNumber, 1, &g_matrixBuffer);

    return true;
}

void render_shader(ID3D11DeviceContext* device_context, int indexCount)
{
    // Set the vertex input layout.
    device_context->IASetInputLayout(g_layout);

    // Set the vertex and pixel shaders that will be used to render this triangle.
    device_context->VSSetShader(g_vertexShader, NULL, 0);
    device_context->PSSetShader(g_pixelShader, NULL, 0);

    // Render the triangle.
    device_context->DrawIndexed(indexCount, 0, 0);
}

bool render(ID3D11DeviceContext* device_context)
{
    device_context->OMSetDepthStencilState(g_depthDisabledStencilState, 1);

    auto world_matrix  = XMMatrixIdentity();

    XMFLOAT3 up(0.0f, 1.0f, 0.0f), position(0.0f, 0.0f, -100.0f), lookAt(0.0f, 0.0f, 0.0f);
    auto view_matrix = XMMatrixLookAtLH(XMLoadFloat3(&position), XMLoadFloat3(&lookAt), XMLoadFloat3(&up));

    auto ortho_matrix = XMMatrixOrthographicLH((float)g_screen_width, (float)g_screen_height, 1.0f, 1000.0f);

    update_buffers(device_context, 0, 0);
    render_buffers(device_context);

    set_shader_parameters(device_context, world_matrix, view_matrix, ortho_matrix);
    render_shader(device_context, g_index_count);

    device_context->OMSetDepthStencilState(g_depthStencilState, 1);
    return true;
}

//
//
//
long __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
#if 1
    static bool init = false;
    static BOOL windowed = true;
    static ID3D11RenderTargetView* pRenderTargetView = nullptr;
    static D3D11_VIEWPORT pViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{ 0 };

    static ID3D11Device* pPreviousDevice = nullptr;

    HRESULT hr = S_OK;
    DXGI_SWAP_CHAIN_DESC dscd;
    hr = pSwapChain->GetDesc(&dscd);

    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pDeviceContext = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = pSwapChain->GetDevice(__uuidof(pDevice), (LPVOID*)&pDevice);
    }
    if (SUCCEEDED(hr))
    {
        pDevice->GetImmediateContext(&pDeviceContext);
    }

    if (!init)
    {
        init = true;
        //MessageBox(0, _T("Boom! It's works!"), _T("Kiero"), MB_OK);
        LOG << "Boom! It's works!\n";

        if (SUCCEEDED(hr))
        {
            LOG << "BufferCount = " << dscd.BufferCount <<
                ", Width = " << dscd.BufferDesc.Width <<
                ", Height = " << dscd.BufferDesc.Height <<
                ", Windowed = " << dscd.Windowed << "\n";

            g_screen_width = dscd.BufferDesc.Width;
            g_screen_height = dscd.BufferDesc.Height;
            windowed = dscd.Windowed;

            init_d3d_resources(pDevice);

            pDeviceContext->OMGetRenderTargets(1, &pRenderTargetView, nullptr);
            if (!pRenderTargetView)
            {
                LOG << "no render target view\n";
            }

            UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            pDeviceContext->RSGetViewports(&numViewports, pViewports);
            if (!numViewports || !pViewports[0].Width)
            {
                LOG << "no viewports\n";

                // Setup viewport
                pViewports[0].Width = (float)800;
                pViewports[0].Height = (float)600;
                pViewports[0].MinDepth = 0.0f;
                pViewports[0].MaxDepth = 1.0f;

                // Set viewport to context
                pDeviceContext->RSSetViewports(1, pViewports);
            }

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
        }
    }
#endif

    if (pPreviousDevice != pDevice)
    {
        LOG << "device pointer is not same!!!\n";
    }

    if (SUCCEEDED(hr))
    {
        pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
        pDeviceContext->RSSetViewports(1, pViewports);
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

    return originalD3D11Present(pSwapChain, SyncInterval, Flags);
    //return S_OK;
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



DWORD WINAPI HookThread(LPVOID lpThreadParameter)
{
    LOG << "enter kieroExampleThread\n";
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

    LOG << "exit kieroExampleThread\n";

    return 0;
}

#if 1

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
    CHAR processPath[MAX_PATH];

    DisableThreadLibraryCalls(hInstance);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        GetModuleFileNameA(GetModuleHandle(NULL), processPath, MAX_PATH);
        LOG << "Inject DLL to " << processPath << "\n";
        CreateThread(NULL, 0, HookThread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        GetModuleFileNameA(GetModuleHandle(NULL), processPath, MAX_PATH);
        LOG << "Unload DLL from " << processPath << "\n";
        if (oEndScene != nullptr)
        {
            kiero::unbind(42);
            oEndScene = nullptr;
        }
        if (originalD3D11Present != nullptr)
        {
            kiero::unbind(8);
            originalD3D11Present = nullptr;
        }
        break;
    }

    return TRUE;
}

#endif
