//--------------------------------------------------------------------------------------
// File: BC6HEncodeDecode.h
//
// Compute Shader Accelerated BC6H Encoder/Decoder
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef __BC6HENCODE_H
#define __BC6HENCODE_H

#pragma once


class CGPUBC6HEncoder
{
public:
    CGPUBC6HEncoder()
        : m_pDevice( NULL ),
        m_pContext( NULL ),
        m_pTryModeG10CS( NULL ),
        m_pTryModeLE10CS( NULL ),
        m_pEncodeBlockCS( NULL ),
        m_pEncodedTextureAsBuf( NULL )
    {}

    HRESULT Initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext );
    void Cleanup();

    //--------------------------------------------------------------------------------------
    // Encode the pSourceTexture to BC6H format using CS acceleration and save it as file 
    // fmtEncode specifies the target texture format to encode to, 
    // must be either DXGI_FORMAT_BC6H_SF16 or DXGI_FORMAT_BC6H_UF16
    //--------------------------------------------------------------------------------------
    HRESULT GPU_BC6HEncodeAndSave( ID3D11Texture2D* pSourceTexture,
                                   DXGI_FORMAT fmtEncode, WCHAR* strDstFilename );

    ID3D11Buffer* GetEncodedTextureAsBuf() { return m_pEncodedTextureAsBuf; }

protected:
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    ID3D11ComputeShader* m_pTryModeG10CS;
    ID3D11ComputeShader* m_pTryModeLE10CS;
    ID3D11ComputeShader* m_pEncodeBlockCS;
    ID3D11Buffer* m_pEncodedTextureAsBuf;    

    HRESULT GPU_BC6HEncode( ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                            ID3D11Texture2D* pSrcTexture, 
                            DXGI_FORMAT dstFormat, ID3D11Buffer** ppDstTextureAsBufOut );
    HRESULT GPU_SaveToFile( ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                            ID3D11Texture2D* pSrcTexture,
                            WCHAR* strFilename,
                            DXGI_FORMAT dstFormat, ID3D11Buffer* pTextureAsBuf );
};

class CGPUBC6HDecoder
{
public:
    CGPUBC6HDecoder()
        : m_pDevice( NULL ),
        m_pContext( NULL ),
        m_pCS( NULL ),
        m_pRestoredTexture( NULL )
    {}

    HRESULT Initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext );
    void Cleanup();
    
    //--------------------------------------------------------------------------------------
    // Decode the pSourceTexture from BC6H format using CS acceleration and save it as file    
    //--------------------------------------------------------------------------------------
    HRESULT GPU_BC6HDecodeAndSave( ID3D11Texture2D* pSourceTexture, WCHAR* strDstFilename );
    HRESULT GPU_BC6HDecodeAndSaveInternal( ID3D11Buffer* pSrcTextureAsBuf, UINT uWidth, UINT uHeight, DXGI_FORMAT fmtSrc, WCHAR* strDstFilename );

    ID3D11Texture2D* GetRestoredTexture() { return m_pRestoredTexture; }

protected:
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    ID3D11ComputeShader* m_pCS;
    ID3D11Texture2D* m_pRestoredTexture;    
};

#endif
