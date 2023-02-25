//-----------------------------------------------------------------------------
// File: skybox11.h
//
// Desc: Encapsulation of skybox geometry and textures
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _SKYBOX11_H
#define _SKYBOX11_H

class CSkybox11
{
public:
    CSkybox11();

    HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, float fSize,
        ID3D11Texture2D* pCubeTexture, ID3D11ShaderResourceView* pCubeRV );

    void    OnD3D11ResizedSwapChain( const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc );
    void    D3D11Render( D3DXMATRIX* pmWorldViewProj, ID3D11DeviceContext* pd3dImmediateContext );
    void    OnD3D11ReleasingSwapChain();
    void    OnD3D11DestroyDevice();

    ID3D11Texture2D* GetD3D10EnvironmentMap()
    {
        return m_pEnvironmentMap11;
    }
    ID3D11ShaderResourceView* GetD3D10EnvironmentMapRV()
    {
        return m_pEnvironmentRV11;
    }
    void    SetD3D11EnvironmentMap( ID3D11Texture2D* pCubeTexture )
    {
        m_pEnvironmentMap11 = pCubeTexture;
    }

protected:
    struct CB_VS_PER_OBJECT
    {
        D3DXMATRIX m_WorldViewProj;
    };    

    ID3D11Texture2D* m_pEnvironmentMap11;
    ID3D11ShaderResourceView* m_pEnvironmentRV11;
    ID3D11Device* m_pd3dDevice11;
    ID3D11VertexShader* m_pVertexShader;
    ID3D11PixelShader* m_pPixelShader;
    ID3D11SamplerState* m_pSam;
    ID3D11InputLayout* m_pVertexLayout11;
    ID3D11Buffer* m_pcbVSPerObject;
    ID3D11Buffer* m_pVB11;
    ID3D11DepthStencilState* m_pDepthStencilState11;

    float m_fSize;
};

#endif