//--------------------------------------------------------------------------------------
// File: ScanCS.cpp
//
// A simple inclusive prefix sum(scan) implemented in CS4.0
// 
// Note, to maintain the simplicity of the sample, this scan has these limitations:
//      - At maximum 16384 elements can be scanned.
//      - The element to be scanned is of type uint2, see comments in ScanCS.hlsl 
//        and below for how to change this type
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "ScanCS.h"

HRESULT CompileShaderFromFile( WCHAR* szFileName, D3D_SHADER_MACRO* pDefines, LPCSTR szEntryPoint, 
                               LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

CScanCS::CScanCS()
    : m_pcbCS(NULL),
        m_pAuxBuf(NULL),
        m_pAuxBufRV(NULL),
        m_pAuxBufUAV(NULL),
        m_pScanCS(NULL),
        m_pScan2CS(NULL)
        
{
}

HRESULT CScanCS::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
    HRESULT hr;

    ID3DBlob* pBlobCS = NULL;
    V_RETURN( CompileShaderFromFile( L"ScanCS.hlsl", NULL, "CSScanInBucket", "cs_4_0", &pBlobCS ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlobCS->GetBufferPointer(), pBlobCS->GetBufferSize(), NULL, &m_pScanCS ) );
    SAFE_RELEASE( pBlobCS );
    DXUT_SetDebugName( m_pScanCS, "CSScanInBucket" );

    V_RETURN( CompileShaderFromFile( L"ScanCS.hlsl", NULL, "CSScanBucketResult", "cs_4_0", &pBlobCS ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlobCS->GetBufferPointer(), pBlobCS->GetBufferSize(), NULL, &m_pScan2CS ) );
    SAFE_RELEASE( pBlobCS );
    DXUT_SetDebugName( m_pScan2CS, "CSScanBucketResult" );

    V_RETURN( CompileShaderFromFile( L"ScanCS.hlsl", NULL, "CSScanAddBucketResult", "cs_4_0", &pBlobCS ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlobCS->GetBufferPointer(), pBlobCS->GetBufferSize(), NULL, &m_pScan3CS ) );
    SAFE_RELEASE( pBlobCS );
    DXUT_SetDebugName( m_pScan3CS, "CSScanAddBucketResult" );

    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;    
    Desc.ByteWidth = sizeof( CB_CS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &m_pcbCS ) );
    DXUT_SetDebugName( m_pcbCS, "CB_CS" );

    ZeroMemory( &Desc, sizeof(Desc) );
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    Desc.StructureByteStride = sizeof(INT) * 2;  // If scan types other than uint2, remember change here
    Desc.ByteWidth = Desc.StructureByteStride * 128;
    Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    V_RETURN( pd3dDevice->CreateBuffer(&Desc, NULL, &m_pAuxBuf) );
    DXUT_SetDebugName( m_pAuxBuf, "Aux" );

    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    ZeroMemory( &DescRV, sizeof( DescRV ) );
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    DescRV.Format = DXGI_FORMAT_UNKNOWN;
    DescRV.Buffer.FirstElement = 0;
    DescRV.Buffer.NumElements = 128;
    V_RETURN( pd3dDevice->CreateShaderResourceView( m_pAuxBuf, &DescRV, &m_pAuxBufRV ) );
    DXUT_SetDebugName( m_pAuxBufRV, "Aux SRV" );

    D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
    ZeroMemory( &DescUAV, sizeof(DescUAV) );
    DescUAV.Format = DXGI_FORMAT_UNKNOWN;
    DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DescUAV.Buffer.FirstElement = 0;
    DescUAV.Buffer.NumElements = 128;
    V_RETURN( pd3dDevice->CreateUnorderedAccessView( m_pAuxBuf, &DescUAV, &m_pAuxBufUAV ) );
    DXUT_SetDebugName( m_pAuxBufUAV, "Aux UAV" );

    return hr;
}

void CScanCS::OnD3D11DestroyDevice()
{
    SAFE_RELEASE( m_pAuxBufRV );
    SAFE_RELEASE( m_pAuxBufUAV );
    SAFE_RELEASE( m_pAuxBuf );
    SAFE_RELEASE( m_pcbCS );
    SAFE_RELEASE( m_pScanCS );
    SAFE_RELEASE( m_pScan2CS );
    SAFE_RELEASE( m_pScan3CS );
}

HRESULT CScanCS::ScanCS( ID3D11DeviceContext* pd3dImmediateContext,
                         INT nNumToScan,
                         ID3D11ShaderResourceView* p0SRV,
                         ID3D11UnorderedAccessView* p0UAV,

                         ID3D11ShaderResourceView* p1SRV,
                         ID3D11UnorderedAccessView* p1UAV )
{
    HRESULT hr = S_OK;    

    // first pass, scan in each bucket
    {
        pd3dImmediateContext->CSSetShader( m_pScanCS, NULL, 0 );

        ID3D11ShaderResourceView* aRViews[ 1 ] = { p0SRV };
        pd3dImmediateContext->CSSetShaderResources( 0, 1, aRViews );

        ID3D11UnorderedAccessView* aUAViews[ 1 ] = { p1UAV };
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, aUAViews, NULL );        

        pd3dImmediateContext->Dispatch( INT(ceil(nNumToScan/128.0f)), 1, 1 );

        ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, NULL );
    }

    // second pass, record and scan the sum of each bucket
    {
        pd3dImmediateContext->CSSetShader( m_pScan2CS, NULL, 0 );

        ID3D11ShaderResourceView* aRViews[ 1 ] = { p1SRV };
        pd3dImmediateContext->CSSetShaderResources( 0, 1, aRViews );

        ID3D11UnorderedAccessView* aUAViews[ 1 ] = { m_pAuxBufUAV };
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, aUAViews, NULL );

        pd3dImmediateContext->Dispatch( 1, 1, 1 );

        ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, NULL );
    }

    // last pass, add the buckets scanned result to each bucket to get the final result
    {
        pd3dImmediateContext->CSSetShader( m_pScan3CS, NULL, 0 );

        ID3D11ShaderResourceView* aRViews[ 2 ] = { p1SRV, m_pAuxBufRV };
        pd3dImmediateContext->CSSetShaderResources( 0, 2, aRViews );

        ID3D11UnorderedAccessView* aUAViews[ 1 ] = { p0UAV };
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, aUAViews, NULL );

        pd3dImmediateContext->Dispatch( INT(ceil(nNumToScan/128.0f)), 1, 1 );
    }

    // Unbind resources for CS
    ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, NULL );
    ID3D11ShaderResourceView* ppSRVNULL[2] = { NULL, NULL };
    pd3dImmediateContext->CSSetShaderResources( 0, 2, ppSRVNULL );
    pd3dImmediateContext->CSSetConstantBuffers( 0, 0, NULL );

    return hr;
}