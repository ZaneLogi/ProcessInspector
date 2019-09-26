#include "stdafx.h"
#include "BitmapFont.h"

bool SaveBitmapImage(const BITMAPINFOHEADER& bi, void* bits, LPCWSTR filename)
{
    BITMAPFILEHEADER   bmfHeader;
    DWORD dwBmpSize = ((bi.biWidth * bi.biBitCount + 31) / 32) * 4 * abs(bi.biHeight);

    // A file is created, this is where we will save the screen capture.
    HANDLE hFile = CreateFile(filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    // Add the size of the headers to the size of the bitmap to get the total file size
    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    //Offset to where the actual bitmap bits start.
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    //Size of the file
    bmfHeader.bfSize = dwSizeofDIB;

    //bfType must always be BM for Bitmaps
    bmfHeader.bfType = 0x4D42; //BM

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)bits, dwBmpSize, &dwBytesWritten, NULL);

    //Close the handle for the file that was created
    CloseHandle(hFile);

    return true;
}

bool BitmapFont::initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    if (!CreateFontTexture(device, deviceContext))
    {
        LOG << "Failed to call CreateFontTexture\n";
        return false;
    }

    return true;
}

void BitmapFont::shutdown()
{
    ReleaseFontTexture();
}

bool BitmapFont::CreateFontTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    HDC hdc = ::CreateCompatibleDC(NULL);

    //Logical units are device dependent pixels, so this will create a handle to a logical font that is 48 pixels in height.
    //The width, when set to 0, will cause the font mapper to choose the closest matching value.
    //The font face name will be Impact.
    HFONT hFont = CreateFont(32, // height
        0, // width
        0, // escapement
        0, // orientation
        FW_DONTCARE, // weight
        FALSE, // italic
        FALSE, // underline
        FALSE, // strikeout
        DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS, // out precision
        CLIP_DEFAULT_PRECIS, // clip precision
        CLEARTYPE_QUALITY, // quality
        VARIABLE_PITCH, // pitch and family
        TEXT("Arial")); // face name
    auto hOldFont = ::SelectObject(hdc, hFont);

    TEXTMETRIC tm;
    ::GetTextMetrics(hdc, &tm);
    m_char_width = tm.tmMaxCharWidth;
    m_char_height = tm.tmHeight;
    m_dib_width = m_char_width * 16;
    m_dib_height = m_char_height * 16;
    int stride = ((((m_dib_width * 32) + 31) & ~31) >> 3);

    BITMAPINFOHEADER bmih =
    {
        sizeof(BITMAPINFOHEADER),
        m_dib_width,
        -m_dib_height, // minus means top-down bitmap
        1, // plane
        32, // bit count
        BI_RGB, // compression
        0, // the size of the image
        0,
        0,
        0,
        0
    };
    PBYTE bits = nullptr;
    HBITMAP hBitmap = ::CreateDIBSection(hdc, (const BITMAPINFO*)&bmih, DIB_RGB_COLORS, (void **)&bits, NULL, 0);

    auto hOldBitmap = ::SelectObject(hdc, hBitmap);

    ::PatBlt(hdc, 0, 0, m_dib_width, m_dib_height, BLACKNESS);
    ::SetTextColor(hdc, RGB(255, 255, 255));
    ::SetBkColor(hdc, RGB(0, 0, 0));

    for (int i = 0; i < 256; i++)
    {
        int x = (i % 16) * m_char_width;
        int y = (i / 16) * m_char_height;
        ::TextOutA(hdc, x, y, (PCSTR)&i, 1);
    }

    GetCharABCWidths(hdc, 0, 255, m_abc);

    //SaveBitmapImage(bmih, bits, L"\\save_image.bmp");

    D3D11_TEXTURE2D_DESC textureDesc;
    textureDesc.Width = m_dib_width;
    textureDesc.Height = m_dib_height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    if (FAILED(device->CreateTexture2D(&textureDesc, NULL, &m_pFontTexture)))
    {
        LOG << "Failed to call ID3D11Device::CreateTexture2D\n";
        return false;
    }

    deviceContext->UpdateSubresource(m_pFontTexture, 0, NULL, bits, stride, 0);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;

    // Create the shader resource view for the texture.
    if (FAILED(device->CreateShaderResourceView(m_pFontTexture, &srvDesc, &m_pFontTextureView)))
    {
        LOG << "Failed to call ID3D11Device::CreateShaderResourceView\n";
        return false;
    }

    ::SelectObject(hdc, hOldBitmap);
    ::DeleteObject(hBitmap);
    ::SelectObject(hdc, hOldFont);
    ::DeleteObject(hFont);
    ::DeleteDC(hdc);
    return true;
}

void BitmapFont::ReleaseFontTexture()
{
    if (m_pFontTexture)
    {
        m_pFontTexture->Release();
        m_pFontTexture = nullptr;
    }

    if (m_pFontTextureView)
    {
        m_pFontTextureView->Release();
        m_pFontTextureView = nullptr;
    }
}

ID3D11ShaderResourceView* BitmapFont::get_texture()
{
    return m_pFontTextureView;
}

void BitmapFont::build_vertex_array(void* vertices, char* sentence, float drawX, float drawY)
{
    VertexType* vertexPtr;
    int numLetters, index;


    // Coerce the input vertices into a VertexType structure.
    vertexPtr = (VertexType*)vertices;

    // Get the number of letters in the sentence.
    numLetters = (int)strlen(sentence);

    // Initialize the index to the vertex array.
    index = 0;

    // Draw each letter onto a quad.
    for (int i = 0; i < numLetters; i++)
    {
        auto letter = sentence[i];
        float bmp_x = (float)(letter % 16) * m_char_width;
        float bmp_y = (float)(letter / 16) * m_char_height;

        PABC char_abc = &m_abc[letter];
        int char_w = char_abc->abcA + char_abc->abcB + char_abc->abcC;

        float u0 = bmp_x / m_dib_width;
        float v0 = bmp_y / m_dib_height;
        float u1 = (bmp_x + char_w) / m_dib_width;
        float v1 = (bmp_y + m_char_height) / m_dib_height;

        // the world matrix is the identity matrix
        // the view matrix is the identity matrix, this means that the camera is position at (0,0,0), looks at (0,0,1), up vector is (0,1,0)
        // the project matrix is the orthogonal matrix, near plan = 1.0f, far plan = 1000.0f
        // set Z = 10.0f to make the triangle visible by the camera
        const float Z = 10.0f;

        // First triangle in quad.
        vertexPtr[index].position = XMFLOAT3(drawX, drawY, Z);  // Top left.
        vertexPtr[index].texture = XMFLOAT2(u0, v0);
        index++;

        vertexPtr[index].position = XMFLOAT3((drawX + char_w), (drawY - m_char_height), Z);  // Bottom right.
        vertexPtr[index].texture = XMFLOAT2(u1, v1);
        index++;

        vertexPtr[index].position = XMFLOAT3(drawX, (drawY - m_char_height), Z);  // Bottom left.
        vertexPtr[index].texture = XMFLOAT2(u0, v1);
        index++;

        // Second triangle in quad.
        vertexPtr[index].position = XMFLOAT3(drawX, drawY, Z);  // Top left.
        vertexPtr[index].texture = XMFLOAT2(u0, v0);
        index++;

        vertexPtr[index].position = XMFLOAT3(drawX + char_w, drawY, Z);  // Top right.
        vertexPtr[index].texture = XMFLOAT2(u1, v0);
        index++;

        vertexPtr[index].position = XMFLOAT3((drawX + char_w), (drawY - m_char_height), Z);  // Bottom right.
        vertexPtr[index].texture = XMFLOAT2(u1, v1);
        index++;

        // Update the x location for drawing by the size of the letter and one pixel.
        drawX = drawX + char_w + 1.0f;
    }
}
