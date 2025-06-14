//--------------------------------------------------------------------------------------
// File: Magnify.cpp
//
// Magnify class implementation. This class magnifies a region of a given surface, and renders a scaled
// sprite at the given position on the screen.
//
// Contributed by AMD Corporation
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "Sprite.h"
#include "Magnify.h"

//===============================================================================================================================//
//
// Constructor(s) / Destructor(s) Block
//
//===============================================================================================================================//

Magnify::Magnify()
{
	// Magnification settings
	m_nPixelRegion = 64;
	m_nHalfPixelRegion = m_nPixelRegion / 2;
	m_nScale = 5;
	m_nPositionX = m_nPixelRegion;
	m_nPositionY = m_nPixelRegion;
	m_fDepthRangeMin = 0.0f;
	m_fDepthRangeMax = 1.0f;
	m_nBackBufferWidth = 0;
	m_nBackBufferHeight = 0;
	m_nSubSampleIndex = 0;

	// Source resource data
	m_pSourceResource = NULL;
	m_pResolvedSourceResource = NULL;
	m_pCopySourceResource = NULL;
	m_pResolvedSourceResourceSRV = NULL;
	m_pCopySourceResourceSRV = NULL;
	m_pSourceResourceSRV1 = NULL;
	m_SourceResourceFormat = DXGI_FORMAT_UNKNOWN;
	m_nSourceResourceWidth = 0;
	m_nSourceResourceHeight = 0;
	m_nSourceResourceSamples = 0;
	m_DepthFormat = DXGI_FORMAT_UNKNOWN;
	m_DepthSRVFormat = DXGI_FORMAT_UNKNOWN;
	m_bDepthFormat = false;
}

Magnify::~Magnify()
{
}

//===============================================================================================================================//
//
// Public functions
//
//===============================================================================================================================//

HRESULT Magnify::OnCreateDevice( ID3D10Device* pd3dDevice )
{
	HRESULT hr;

	assert( NULL != pd3dDevice );

	hr = m_Sprite.OnCreateDevice( pd3dDevice );
	assert( D3D_OK == hr );
		
	return hr;
}

void Magnify::OnDestroyDevice()
{
	m_Sprite.OnDestroyDevice();

	SAFE_RELEASE( m_pResolvedSourceResourceSRV );
	SAFE_RELEASE( m_pCopySourceResourceSRV );
	SAFE_RELEASE( m_pSourceResourceSRV1 );
	SAFE_RELEASE( m_pResolvedSourceResource );
	SAFE_RELEASE( m_pCopySourceResource );
}

void Magnify::OnResizedSwapChain( const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc )
{
	assert( NULL != pBackBufferSurfaceDesc );

    if( pBackBufferSurfaceDesc == NULL )
        return;

	m_Sprite.OnResizedSwapChain( pBackBufferSurfaceDesc );

	m_nBackBufferWidth = pBackBufferSurfaceDesc->Width;
	m_nBackBufferHeight = pBackBufferSurfaceDesc->Height;
}

void Magnify::Capture()
{
	POINT Point;
	::GetCursorPos( &Point );

	RECT Rect;
	::GetWindowRect( DXUTGetHWND(), &Rect );

	int nWidthDiff = 0;
	int nHeightDiff = 0;
	
	if( DXUTIsWindowed() )
	{
		nWidthDiff = (int)( ( ( Rect.right - Rect.left ) - m_nBackBufferWidth ) * ( 1.0f / 2.0f ) );
		nHeightDiff = (int)( ( ( Rect.bottom - Rect.top ) - m_nBackBufferHeight ) * ( 4.0f / 5.0f ) );
	}
		
	SetPosition( Point.x - ( Rect.left + nWidthDiff ), Point.y - ( Rect.top + nHeightDiff ) );

	D3D10_BOX SourceRegion;
	SourceRegion.left = m_nPositionX - m_nHalfPixelRegion;
	SourceRegion.right = m_nPositionX + m_nHalfPixelRegion;
	SourceRegion.top = m_nPositionY - m_nHalfPixelRegion;
	SourceRegion.bottom = m_nPositionY + m_nHalfPixelRegion;
	SourceRegion.front = 0;
	SourceRegion.back = 1;
}

void Magnify::SetSourceResource( ID3D10Resource* pSourceResource, DXGI_FORMAT Format,
		int nWidth, int nHeight, int nSamples )
{
	assert( NULL != pSourceResource );
	assert( Format > DXGI_FORMAT_UNKNOWN );
	assert( nWidth > 0 );
	assert( nHeight > 0 );
	assert( nSamples > 0 );

	m_pSourceResource = pSourceResource;
	m_SourceResourceFormat = Format;
	m_nSourceResourceWidth = nWidth;
	m_nSourceResourceHeight = nHeight;
	m_nSourceResourceSamples = nSamples;

	m_bDepthFormat = false;

	switch( m_SourceResourceFormat )
	{
		case DXGI_FORMAT_D32_FLOAT:
			m_DepthFormat = DXGI_FORMAT_R32_TYPELESS;
			m_DepthSRVFormat = DXGI_FORMAT_R32_FLOAT;
			m_bDepthFormat = true;
			break;

		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			m_DepthFormat = DXGI_FORMAT_R24G8_TYPELESS;
			m_DepthSRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			m_bDepthFormat = true;
			break;

		case DXGI_FORMAT_D16_UNORM:
			m_DepthFormat = DXGI_FORMAT_R16_TYPELESS;
			m_DepthSRVFormat = DXGI_FORMAT_R16_UNORM;
			m_bDepthFormat = true;
			break;
	}

	CreateInternalResources();	
}

void Magnify::SetPixelRegion( int nPixelRegion )
{
	assert( nPixelRegion > 1 );

	m_nPixelRegion = nPixelRegion;
	m_nHalfPixelRegion = m_nPixelRegion / 2;
}

void Magnify::SetScale( int nScale )
{
	assert( nScale > 1 );

	m_nScale = nScale;
}

void Magnify::SetDepthRangeMin( float fDepthRangeMin )
{
	m_fDepthRangeMin = fDepthRangeMin;
}
	
void Magnify::SetDepthRangeMax( float fDepthRangeMax )
{
	m_fDepthRangeMax = fDepthRangeMax;
}

void Magnify::SetSubSampleIndex( int nSubSampleIndex )
{
	m_nSubSampleIndex = nSubSampleIndex;
}

void Magnify::RenderBackground()
{
	if( m_bDepthFormat )
	{
		if( m_nSourceResourceSamples == 1 )
		{
			DXUTGetD3D10Device()->CopyResource( m_pCopySourceResource, m_pSourceResource );
		}
	}
	else
	{
		if( m_nSourceResourceSamples > 1 )
		{
			DXUTGetD3D10Device()->ResolveSubresource( m_pResolvedSourceResource, 0,
				m_pSourceResource, 0, m_SourceResourceFormat );
		}
		else
		{
			DXUTGetD3D10Device()->CopyResource( m_pCopySourceResource, m_pSourceResource );

		}
	}

	if( m_bDepthFormat )
	{
		if( m_nSourceResourceSamples > 1 )
		{
			// Get the current render and depth targets, so we can later revert to these.
			ID3D10RenderTargetView *pRenderTargetView;
			ID3D10DepthStencilView *pDepthStencilView;
			DXUTGetD3D10Device()->OMGetRenderTargets( 1, &pRenderTargetView, &pDepthStencilView );

			// Bind our render target view to the OM stage.
			DXUTGetD3D10Device()->OMSetRenderTargets( 1, (ID3D10RenderTargetView* const*)&pRenderTargetView, NULL );
			
			m_Sprite.SetUVs( 0.0f, 0.0f, 1.0f, 1.0f );

			m_Sprite.RenderSpriteAsDepthMS( m_pSourceResourceSRV1, 0, m_nBackBufferHeight,
				m_nBackBufferWidth, m_nBackBufferHeight, m_nBackBufferWidth, m_nBackBufferHeight,
				false, m_fDepthRangeMin, m_fDepthRangeMax, m_nSubSampleIndex );

			// Bind back to the original render, depth target, and viewport
			DXUTGetD3D10Device()->OMSetRenderTargets( 1, (ID3D10RenderTargetView* const*)&pRenderTargetView, pDepthStencilView );

			// Decrement the counter on these resources
			SAFE_RELEASE( pRenderTargetView );
			SAFE_RELEASE( pDepthStencilView );
		}
		else
		{
			m_Sprite.SetUVs( 0.0f, 0.0f, 1.0f, 1.0f );

			m_Sprite.RenderSpriteAsDepth( m_pCopySourceResourceSRV, 0, m_nBackBufferHeight,
				m_nBackBufferWidth, m_nBackBufferHeight, false, m_fDepthRangeMin, m_fDepthRangeMax );
		}
	}
	else
	{
		m_Sprite.SetUVs( 0.0f, 0.0f, 1.0f, 1.0f );

		if( m_nSourceResourceSamples > 1 )
		{
			m_Sprite.RenderSprite( m_pResolvedSourceResourceSRV, 0, m_nBackBufferHeight,
				m_nBackBufferWidth, m_nBackBufferHeight, false, false );
		}
		else
		{
			m_Sprite.RenderSprite( m_pCopySourceResourceSRV, 0, m_nBackBufferHeight,
				m_nBackBufferWidth, m_nBackBufferHeight, false, false );
		}
	}
}

void Magnify::RenderMagnifiedRegion()
{
	m_Sprite.SetUVs( ( m_nPositionX - m_nHalfPixelRegion ) / (float)m_nSourceResourceWidth,
		( m_nPositionY - m_nHalfPixelRegion ) / (float)m_nSourceResourceHeight,
		( m_nPositionX + m_nHalfPixelRegion ) / (float)m_nSourceResourceWidth,
		( m_nPositionY + m_nHalfPixelRegion ) / (float)m_nSourceResourceHeight );

	if( m_bDepthFormat )
	{
		D3DXCOLOR Color( 0xffffffff );
		m_Sprite.SetBorderColor( Color );

		if( m_nSourceResourceSamples > 1 )
		{
			// Get the current render and depth targets, so we can later revert to these
			ID3D10RenderTargetView *pRenderTargetView;
			ID3D10DepthStencilView *pDepthStencilView;
			DXUTGetD3D10Device()->OMGetRenderTargets( 1, &pRenderTargetView, &pDepthStencilView );

			// Bind our render target view to the OM stage
			DXUTGetD3D10Device()->OMSetRenderTargets( 1, (ID3D10RenderTargetView* const*)&pRenderTargetView, NULL );
			
			m_Sprite.RenderSpriteAsDepthMS( m_pSourceResourceSRV1, (m_nPositionX - m_nHalfPixelRegion * m_nScale),
				(m_nPositionY + m_nHalfPixelRegion * m_nScale), (m_nPixelRegion * m_nScale),
				(m_nPixelRegion * m_nScale), m_nSourceResourceWidth, m_nSourceResourceHeight, true,
				m_fDepthRangeMin, m_fDepthRangeMax, m_nSubSampleIndex );

			// Bind back to the original render, depth target, and viewport
			DXUTGetD3D10Device()->OMSetRenderTargets( 1, (ID3D10RenderTargetView* const*)&pRenderTargetView, pDepthStencilView );

			// Decrement the counter on these resources
			SAFE_RELEASE( pRenderTargetView );
			SAFE_RELEASE( pDepthStencilView );
		}
		else
		{
			m_Sprite.RenderSpriteAsDepth( m_pCopySourceResourceSRV, (m_nPositionX - m_nHalfPixelRegion * m_nScale),
				(m_nPositionY + m_nHalfPixelRegion * m_nScale), (m_nPixelRegion * m_nScale),
				(m_nPixelRegion * m_nScale), true, m_fDepthRangeMin, m_fDepthRangeMax );
		}
	}
    else
    {
        D3DXCOLOR Color( 0xff000000 );
        m_Sprite.SetBorderColor( Color );

        if( m_nSourceResourceSamples > 1 )
        {
            m_Sprite.RenderSprite( m_pResolvedSourceResourceSRV, (m_nPositionX - m_nHalfPixelRegion * m_nScale),
                (m_nPositionY + m_nHalfPixelRegion * m_nScale), (m_nPixelRegion * m_nScale),
                (m_nPixelRegion * m_nScale), false, true );
        }
        else
        {
            m_Sprite.RenderSprite( m_pCopySourceResourceSRV, (m_nPositionX - m_nHalfPixelRegion * m_nScale),
                (m_nPositionY + m_nHalfPixelRegion * m_nScale), (m_nPixelRegion * m_nScale),
                (m_nPixelRegion * m_nScale), false, true );
        }
    }
}

//===============================================================================================================================//
//
// Protected functions
//
//===============================================================================================================================//

//===============================================================================================================================//
//
// Private functions
//
//===============================================================================================================================//

void Magnify::SetPosition( int nPositionX, int nPositionY )
{
    m_nPositionX = nPositionX;
    m_nPositionY = nPositionY;

    int nMinX = m_nPixelRegion;
    int nMaxX = m_nSourceResourceWidth - m_nPixelRegion;
    int nMinY = m_nPixelRegion;
    int nMaxY = m_nSourceResourceHeight - m_nPixelRegion;

    m_nPositionX = ( m_nPositionX < nMinX ) ? ( nMinX ) : ( m_nPositionX );
    m_nPositionX = ( m_nPositionX > nMaxX ) ? ( nMaxX ) : ( m_nPositionX );

    m_nPositionY = ( m_nPositionY < nMinY ) ? ( nMinY ) : ( m_nPositionY );
    m_nPositionY = ( m_nPositionY > nMaxY ) ? ( nMaxY ) : ( m_nPositionY );
}

void Magnify::CreateInternalResources()
{
    HRESULT hr;

    SAFE_RELEASE( m_pResolvedSourceResourceSRV );
    SAFE_RELEASE( m_pCopySourceResourceSRV );
    SAFE_RELEASE( m_pSourceResourceSRV1 );
    SAFE_RELEASE( m_pResolvedSourceResource );
    SAFE_RELEASE( m_pCopySourceResource );

    D3D10_TEXTURE2D_DESC Desc;
    ZeroMemory( &Desc, sizeof( Desc ) );
    Desc.Width = m_nBackBufferWidth;
    Desc.Height = m_nBackBufferHeight;
    Desc.MipLevels = 1;
    Desc.ArraySize = 1;
    Desc.Format = ( m_bDepthFormat ) ? ( m_DepthFormat ) : ( m_SourceResourceFormat );
    Desc.SampleDesc.Count = 1;
    Desc.Usage = D3D10_USAGE_DEFAULT;
    Desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;

    hr = DXUTGetD3D10Device()->CreateTexture2D( &Desc, NULL, &m_pResolvedSourceResource );
    assert( D3D_OK == hr );

    Desc.SampleDesc.Count = m_nSourceResourceSamples;

    hr = DXUTGetD3D10Device()->CreateTexture2D( &Desc, NULL, &m_pCopySourceResource );
    assert( D3D_OK == hr );

    if( m_bDepthFormat )
    {
        if( m_nSourceResourceSamples > 1 )
        {
            if( NULL != DXUTGetD3D10Device1() )
            {
                D3D10_SHADER_RESOURCE_VIEW_DESC1 SRDesc1;
                SRDesc1.Format = m_DepthSRVFormat;
                SRDesc1.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE2DMS;

                hr = DXUTGetD3D10Device1()->CreateShaderResourceView1( m_pSourceResource, &SRDesc1, &m_pSourceResourceSRV1 );
                assert( D3D_OK == hr );
            }
        }
        else
        {
            D3D10_SHADER_RESOURCE_VIEW_DESC SRDesc;
            SRDesc.Format = m_DepthSRVFormat;
            SRDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
            SRDesc.Texture2D.MostDetailedMip = 0;
            SRDesc.Texture2D.MipLevels = 1;

            hr = DXUTGetD3D10Device()->CreateShaderResourceView( m_pCopySourceResource, &SRDesc, &m_pCopySourceResourceSRV );
            assert( D3D_OK == hr );
        }
    }
    else
    {
        if( m_nSourceResourceSamples > 1 )
        {
            hr = DXUTGetD3D10Device()->CreateShaderResourceView( m_pResolvedSourceResource, NULL, &m_pResolvedSourceResourceSRV );
            assert( D3D_OK == hr );
        }
        else
        {
            hr = DXUTGetD3D10Device()->CreateShaderResourceView( m_pCopySourceResource, NULL, &m_pCopySourceResourceSRV );
            assert( D3D_OK == hr );
        }
    }
}

//=================================================================================================================================
// EOF.
//=================================================================================================================================




