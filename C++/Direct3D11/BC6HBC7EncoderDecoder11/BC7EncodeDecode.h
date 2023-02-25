//--------------------------------------------------------------------------------------
// File: BC7EncodeDecode.h
//
// Compute Shader Accelerated BC7 Encoder/Decoder
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef __BC7ENCODE_H
#define __BC7ENCODE_H

#pragma once


class CGPUBC7Encoder
{
public:
    CGPUBC7Encoder()
        : m_pDevice( NULL ),
        m_pContext( NULL ),        
        m_pTryMode456CS( NULL ),
        m_pTryMode137CS( NULL ),
        m_pTryMode02CS( NULL ),
        m_pEncodeBlockCS( NULL ),
        m_pEncodedTextureAsBuf( NULL )
    {}

    HRESULT Initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext );
    void Cleanup();

    //--------------------------------------------------------------------------------------
    // Encode the pSourceTexture to BC7 format using CS acceleration and save it as file
    // fmtEncode specifies the target texture format to encode to, 
    // must be DXGI_FORMAT_BC7_UNORM 
    //--------------------------------------------------------------------------------------
    HRESULT GPU_BC7EncodeAndSave( ID3D11Texture2D* pSourceTexture,
                                  DXGI_FORMAT fmtEncode, WCHAR* strDstFilename );

    ID3D11Buffer* GetEncodedTextureAsBuf() { return m_pEncodedTextureAsBuf; }

protected:
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    ID3D11ComputeShader* m_pTryMode456CS;
    ID3D11ComputeShader* m_pTryMode137CS;
    ID3D11ComputeShader* m_pTryMode02CS;
    ID3D11ComputeShader* m_pEncodeBlockCS;
    ID3D11Buffer* m_pEncodedTextureAsBuf;    

    HRESULT GPU_BC7Encode( ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                           ID3D11Texture2D* pSrcTexture, 
                           DXGI_FORMAT dstFormat, ID3D11Buffer** ppDstTextureAsBufOut );
    HRESULT GPU_SaveToFile( ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                            ID3D11Texture2D* pSrcTexture,
                            WCHAR* strFilename,
                            DXGI_FORMAT dstFormat, ID3D11Buffer* pTextureAsBuf );
};

class CGPUBC7Decoder
{
public:
    CGPUBC7Decoder()
        : m_pDevice( NULL ),
        m_pContext( NULL ),
        m_pCS( NULL ),
        m_pRestoredTexture( NULL )
    {}

    HRESULT Initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext );
    void Cleanup();
    
    //--------------------------------------------------------------------------------------
    // Decode the pSourceTexture from BC7 format using CS acceleration and save it as file    
    //--------------------------------------------------------------------------------------
    HRESULT GPU_BC7DecodeAndSave( ID3D11Texture2D* pSourceTexture, WCHAR* strDstFilename );
    HRESULT GPU_BC7DecodeAndSaveInternal( ID3D11Buffer* pSrcTextureAsBuf, UINT uWidth, UINT uHeight, DXGI_FORMAT fmtSrc, WCHAR* strDstFilename );

    ID3D11Texture2D* GetRestoredTexture() { return m_pRestoredTexture; }

protected:
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    ID3D11ComputeShader* m_pCS;
    ID3D11Texture2D* m_pRestoredTexture;    
};

#endif
