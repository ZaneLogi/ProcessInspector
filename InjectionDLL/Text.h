#pragma once

#include "BitmapFont.h"
#include "BitmapFontShader.h"

class Text
{
private:
    struct SentenceType
    {
        ID3D11Buffer *vertexBuffer, *indexBuffer;
        int vertexCount, indexCount, maxLength;
        float red, green, blue;
    };

    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

public:
    Text() = default;
    Text(const Text&) = delete;

    bool initialize(ID3D11Device*, ID3D11DeviceContext*, HWND, int, int);
    void shutdown();
    bool render(ID3D11DeviceContext*, const XMMATRIX&);

private:
    bool InitializeSentence(SentenceType**, int, ID3D11Device*);
    bool UpdateSentence(SentenceType*, char*, int, int, float, float, float, ID3D11DeviceContext*);
    void ReleaseSentence(SentenceType**);
    bool RenderSentence(ID3D11DeviceContext*, SentenceType*, const XMMATRIX&);

private:
    BitmapFont* m_Font = nullptr;
    BitmapFontShader* m_FontShader = nullptr;
    int m_screenWidth, m_screenHeight;

    SentenceType* m_sentence1 = nullptr;
    SentenceType* m_sentence2 = nullptr;
};
