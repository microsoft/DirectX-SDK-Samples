//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.cpp
//
// Functions for loading a DDS texture without using D3DX
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DDSTextureLoader.h"
#include "DDS.h"

#include <new>

using namespace DirectX;

//--------------------------------------------------------------------------------------
static HRESULT LoadTextureDataFromFile( __in_z const WCHAR* szFileName, BYTE** ppHeapData,
                                        DDS_HEADER** ppHeader,
                                        BYTE** ppBitData, UINT* pBitSize )
{
    // open the file
    HANDLE hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN, NULL );
    if( INVALID_HANDLE_VALUE == hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Get the file size
    LARGE_INTEGER FileSize = {};
    GetFileSizeEx( hFile, &FileSize );

    // File is too big for 32-bit allocation, so reject read
    if( FileSize.HighPart > 0 )
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    // Need at least enough data to fill the header and magic number to be a valid DDS
    if ( FileSize.LowPart < DDS_MIN_HEADER_SIZE )
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    // create enough space for the file data
    *ppHeapData = new (std::nothrow) BYTE[FileSize.LowPart];
    if( !( *ppHeapData ) )
    {
        CloseHandle( hFile );
        return E_OUTOFMEMORY;
    }

    // read the data in
    DWORD BytesRead = 0;
    if( !ReadFile( hFile, *ppHeapData, FileSize.LowPart, &BytesRead, NULL ) )
    {
        CloseHandle( hFile );
        SAFE_DELETE_ARRAY( *ppHeapData );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if( BytesRead < FileSize.LowPart )
    {
        CloseHandle( hFile );
        SAFE_DELETE_ARRAY( *ppHeapData );
        return E_FAIL;
    }

    // DDS files always start with the same magic number ("DDS ")
    DWORD dwMagicNumber = *( DWORD* )( *ppHeapData );
    if( dwMagicNumber != DDS_MAGIC )
    {
        CloseHandle( hFile );
        SAFE_DELETE_ARRAY( *ppHeapData );
        return E_FAIL;
    }

    DDS_HEADER* pHeader = reinterpret_cast<DDS_HEADER*>( *ppHeapData + sizeof( DWORD ) );

    // Verify header to validate DDS file
    if( pHeader->size != sizeof(DDS_HEADER)
        || pHeader->ddspf.size != sizeof(DDS_PIXELFORMAT) )
    {
        CloseHandle( hFile );
        SAFE_DELETE_ARRAY( *ppHeapData );
        return E_FAIL;
    }

    // Check for DX10 extension
    bool bDXT10Header = false;
    if ( (pHeader->ddspf.flags & DDS_FOURCC)
        && (MAKEFOURCC( 'D', 'X', '1', '0' ) == pHeader->ddspf.fourCC) )
    {
        // Must be long enough for both headers and magic value
        if (FileSize.LowPart < DDS_DX10_HEADER_SIZE)
        {
            CloseHandle( hFile );
            SAFE_DELETE_ARRAY( *ppHeapData );
            return E_FAIL;
        }

        bDXT10Header = true;
    }

    // setup the pointers in the process request
    *ppHeader = pHeader;
    INT offset = DDS_MIN_HEADER_SIZE
                 + (bDXT10Header ? sizeof( DDS_HEADER_DXT10 ) : 0);
    *ppBitData = *ppHeapData + offset;
    *pBitSize = FileSize.LowPart - offset;

    CloseHandle( hFile );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
static UINT BitsPerPixel( D3DFORMAT fmt )
{
    UINT fmtU = ( UINT )fmt;
    switch( fmtU )
    {
        case D3DFMT_A32B32G32R32F:
            return 128;

        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_G32R32F:
            return 64;

        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A8B8G8R8:
        case D3DFMT_X8B8G8R8:
        case D3DFMT_G16R16:
        case D3DFMT_A2R10G10B10:
        case D3DFMT_Q8W8V8U8:
        case D3DFMT_V16U16:
        case D3DFMT_X8L8V8U8:
        case D3DFMT_A2W10V10U10:
        case D3DFMT_D32:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D24X4S4:
        case D3DFMT_D32F_LOCKABLE:
        case D3DFMT_D24FS8:
        case D3DFMT_INDEX32:
        case D3DFMT_G16R16F:
        case D3DFMT_R32F:
            return 32;

        case D3DFMT_R8G8B8:
            return 24;

        case D3DFMT_A4R4G4B4:
        case D3DFMT_X4R4G4B4:
        case D3DFMT_R5G6B5:
        case D3DFMT_L16:
        case D3DFMT_A8L8:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_V8U8:
        case D3DFMT_CxV8U8:
        case D3DFMT_L6V5U5:
        case D3DFMT_G8R8_G8B8:
        case D3DFMT_R8G8_B8G8:
        case D3DFMT_D16_LOCKABLE:
        case D3DFMT_D15S1:
        case D3DFMT_D16:
        case D3DFMT_INDEX16:
        case D3DFMT_R16F:
        case D3DFMT_YUY2:
            return 16;

        case D3DFMT_R3G3B2:
        case D3DFMT_A8:
        case D3DFMT_A8P8:
        case D3DFMT_P8:
        case D3DFMT_L8:
        case D3DFMT_A4L4:
            return 8;

        case D3DFMT_DXT1:
            return 4;

        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            return  8;

            // From DX docs, reference/d3d/enums/d3dformat.asp
            // (note how it says that D3DFMT_R8G8_B8G8 is "A 16-bit packed RGB format analogous to UYVY (U0Y0, V0Y1, U2Y2, and so on)")
        case D3DFMT_UYVY:
            return 16;

            // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directshow/htm/directxvideoaccelerationdxvavideosubtypes.asp
        case MAKEFOURCC( 'A', 'I', '4', '4' ):
        case MAKEFOURCC( 'I', 'A', '4', '4' ):
            return 8;

        case MAKEFOURCC( 'Y', 'V', '1', '2' ):
            return 12;

#if !defined(D3D_DISABLE_9EX)
        case D3DFMT_D32_LOCKABLE:
            return 32;

        case D3DFMT_S8_LOCKABLE:
            return 8;

        case D3DFMT_A1:
            return 1;
#endif // !D3D_DISABLE_9EX

        default:
            return 0;
    }
}

static UINT BitsPerPixel( DXGI_FORMAT fmt )
{
    switch( fmt )
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return 32;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return 16;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    default:
        return 0;
    }
}


//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
static void GetSurfaceInfo( UINT width, UINT height, D3DFORMAT fmt, UINT* pNumBytes, UINT* pRowBytes, UINT* pNumRows )
{
    UINT numBytes = 0;
    UINT rowBytes = 0;
    UINT numRows = 0;

    // From the DXSDK docs:
    //
    //     When computing DXTn compressed sizes for non-square textures, the
    //     following formula should be used at each mipmap level:
    //
    //         max(1, width / 4) x max(1, height / 4) x 8(DXT1) or 16(DXT2-5)
    //
    //     The pitch for DXTn formats is different from what was returned in
    //     Microsoft DirectX 7.0. It now refers the pitch of a row of blocks.
    //     For example, if you have a width of 16, then you will have a pitch
    //     of four blocks (4*8 for DXT1, 4*16 for DXT2-5.)"

    switch( fmt )
    {
        case D3DFMT_DXT1:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            {
                int numBlocksWide = 0;
                if( width > 0 )
                    numBlocksWide = max( 1, (width + 3) / 4 );
                int numBlocksHigh = 0;
                if( height > 0 )
                    numBlocksHigh = max( 1, (height + 3) / 4 );
                int numBytesPerBlock = ( fmt == D3DFMT_DXT1 ? 8 : 16 );
                rowBytes = numBlocksWide * numBytesPerBlock;
                numRows = numBlocksHigh;
            }
            break;

        case D3DFMT_R8G8_B8G8:
        case D3DFMT_G8R8_G8B8:
        case D3DFMT_UYVY:
        case D3DFMT_YUY2:
            {
                rowBytes = ( ( width + 1 ) >> 1 ) * 4;
                numRows = height;
            }
            break;

        default:
            {
                UINT bpp = BitsPerPixel( fmt );
                rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
                numRows = height;
            }
            break;
    }

    numBytes = rowBytes * numRows;
    if( pNumBytes != NULL )
        *pNumBytes = numBytes;
    if( pRowBytes != NULL )
        *pRowBytes = rowBytes;
    if( pNumRows != NULL )
        *pNumRows = numRows;
}

static void GetSurfaceInfo( UINT width, UINT height, DXGI_FORMAT fmt, UINT* pNumBytes, UINT* pRowBytes, UINT* pNumRows )
{
    UINT numBytes = 0;
    UINT rowBytes = 0;
    UINT numRows = 0;

    bool bc = false;
    bool packed  = false;
    UINT bcnumBytesPerBlock = 0;
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        bc=true;
        bcnumBytesPerBlock = 8;
        break;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        bc = true;
        bcnumBytesPerBlock = 16;
        break;

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
        packed = true;
        break;
    }

    if( bc )
    {
        int numBlocksWide = 0;
        if( width > 0 )
            numBlocksWide = max( 1, (width + 3) / 4 );
        int numBlocksHigh = 0;
        if( height > 0 )
            numBlocksHigh = max( 1, (height + 3) / 4 );
        rowBytes = numBlocksWide * bcnumBytesPerBlock;
        numRows = numBlocksHigh;
    }
    else if ( packed )
    {
        rowBytes = ( ( width + 1 ) >> 1 ) * 4;
        numRows = height;
    }
    else
    {
        UINT bpp = BitsPerPixel( fmt );
        rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
        numRows = height;
    }

    numBytes = rowBytes * numRows;
    if( pNumBytes != NULL )
        *pNumBytes = numBytes;
    if( pRowBytes != NULL )
        *pRowBytes = rowBytes;
    if( pNumRows != NULL )
        *pNumRows = numRows;
}


//--------------------------------------------------------------------------------------
#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )

//--------------------------------------------------------------------------------------
static D3DFORMAT GetD3D9Format( const DDS_PIXELFORMAT& ddpf )
{
    if( ddpf.flags & DDS_RGB )
    {
        switch (ddpf.RGBBitCount)
        {
        case 32:
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0xff000000) )
                return D3DFMT_A8R8G8B8;
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000) )
                return D3DFMT_X8R8G8B8;
            if( ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000) )
                return D3DFMT_A8B8G8R8;
            if( ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) )
                return D3DFMT_X8B8G8R8;

            // Note that many common DDS reader/writers (including D3DX) swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assumme
            // below that the 'incorrect' header mask is being used

            // For 'correct' writers this should be 0x3ff00000,0x000ffc00,0x000003ff for BGR data
            if( ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) )
                return D3DFMT_A2R10G10B10;

            // For 'correct' writers this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
            if( ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000) )
                return D3DFMT_A2B10G10R10;

            if( ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000) )
                return D3DFMT_G16R16;
            if( ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000) )
                return D3DFMT_R32F; // D3DX writes this out as a FourCC of 114
            break;

        case 24:
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000) )
                return D3DFMT_R8G8B8;
            break;

        case 16:
            if( ISBITMASK(0x0000f800,0x000007e0,0x0000001f,0x00000000) )
                return D3DFMT_R5G6B5;
            if( ISBITMASK(0x00007c00,0x000003e0,0x0000001f,0x00008000) )
                return D3DFMT_A1R5G5B5;
            if( ISBITMASK(0x00007c00,0x000003e0,0x0000001f,0x00000000) )
                return D3DFMT_X1R5G5B5;
            if( ISBITMASK(0x00000f00,0x000000f0,0x0000000f,0x0000f000) )
                return D3DFMT_A4R4G4B4;
            if( ISBITMASK(0x00000f00,0x000000f0,0x0000000f,0x00000000) )
                return D3DFMT_X4R4G4B4;

            if (ISBITMASK(0x00e0, 0x001c, 0x0003, 0xff00))
                return D3DFMT_A8R3G3B2;

            // NVTT versions 1.x wrote these as RGB instead of LUMINANCE
            if (ISBITMASK(0xffff, 0, 0, 0))
                return D3DFMT_L16;
            if (ISBITMASK(0x00ff, 0, 0, 0xff00))
                return D3DFMT_A8L8;
            break;

        case 8:
            if (ISBITMASK(0xe0, 0x1c, 0x03, 0))
                return D3DFMT_R3G3B2;

            // NVTT versions 1.x wrote these as RGB instead of LUMINANCE
            if (ISBITMASK(0xff, 0, 0, 0))
                return D3DFMT_L8;

            // Paletted texture formats are typically not supported on modern video cards aka D3DFMT_P8, D3DFMT_A8P8
            break;

        default:
            return D3DFMT_UNKNOWN;
        }
    }
    else if( ddpf.flags & DDS_LUMINANCE )
    {
        switch (ddpf.RGBBitCount)
        {
        case 16:
            if (ISBITMASK(0xffff, 0, 0, 0))
                return D3DFMT_L16;
            if (ISBITMASK(0x00ff, 0, 0, 0xff00))
                return D3DFMT_A8L8;
            break;

        case 8:
            if (ISBITMASK(0x0f, 0, 0, 0xf0))
                return D3DFMT_A4L4;
            if (ISBITMASK(0xff, 0, 0, 0))
                return D3DFMT_L8;

            if (ISBITMASK(0x00ff, 0, 0, 0xff00))
                return D3DFMT_A8L8; // Some DDS writers assume the bitcount should be 8 instead of 16
            break;

        default:
            return D3DFMT_UNKNOWN;
        }
    }
    else if( ddpf.flags & DDS_ALPHA )
    {
        if( 8 == ddpf.RGBBitCount )
        {
            return D3DFMT_A8;
        }
    }
    else if (ddpf.flags & DDS_BUMPDUDV)
    {
        switch (ddpf.RGBBitCount)
        {
        case 32:
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                return D3DFMT_Q8W8V8U8;
            if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
                return D3DFMT_V16U16;
            if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
                return D3DFMT_A2W10V10U10;
            break;

        case 16:
            if (ISBITMASK(0x00ff, 0xff00, 0, 0))
                return D3DFMT_V8U8;
            break;

        default:
            return D3DFMT_UNKNOWN;
        }
    }
    else if (ddpf.flags & DDS_BUMPLUMINANCE)
    {
        switch (ddpf.RGBBitCount)
        {
        case 32:
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0))
                return D3DFMT_X8L8V8U8;
            break;

        case 16:
            if (ISBITMASK(0x001f, 0x03e0, 0xfc00, 0))
                return D3DFMT_L6V5U5;
            break;

        default:
            return D3DFMT_UNKNOWN;
        }
    }
    else if( ddpf.flags & DDS_FOURCC )
    {
        if( MAKEFOURCC( 'D', 'X', 'T', '1' ) == ddpf.fourCC )
            return D3DFMT_DXT1;
        if( MAKEFOURCC( 'D', 'X', 'T', '2' ) == ddpf.fourCC )
            return D3DFMT_DXT2;
        if( MAKEFOURCC( 'D', 'X', 'T', '3' ) == ddpf.fourCC )
            return D3DFMT_DXT3;
        if( MAKEFOURCC( 'D', 'X', 'T', '4' ) == ddpf.fourCC )
            return D3DFMT_DXT4;
        if( MAKEFOURCC( 'D', 'X', 'T', '5' ) == ddpf.fourCC )
            return D3DFMT_DXT5;

        if( MAKEFOURCC( 'R', 'G', 'B', 'G' ) == ddpf.fourCC )
            return D3DFMT_R8G8_B8G8;
        if( MAKEFOURCC( 'G', 'R', 'G', 'B' ) == ddpf.fourCC )
            return D3DFMT_G8R8_G8B8;

        if( MAKEFOURCC( 'U', 'Y', 'V', 'Y' ) == ddpf.fourCC )
            return D3DFMT_UYVY;
        if( MAKEFOURCC( 'Y', 'U', 'Y', '2' ) == ddpf.fourCC )
            return D3DFMT_YUY2;

        // Check for D3DFORMAT enums being set here
        switch( ddpf.fourCC )
        {
        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_R16F:
        case D3DFMT_G16R16F:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_R32F:
        case D3DFMT_G32R32F:
        case D3DFMT_A32B32G32R32F:
        case D3DFMT_CxV8U8:
            return static_cast<D3DFORMAT>(ddpf.fourCC);

        default:
            return D3DFMT_UNKNOWN;
        }
    }

    return D3DFMT_UNKNOWN;
}

//--------------------------------------------------------------------------------------
static DXGI_FORMAT GetDXGIFormat( const DDS_PIXELFORMAT& ddpf )
{
    if( ddpf.flags & DDS_RGB )
    {
        switch (ddpf.RGBBitCount)
        {
        case 32:
            // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB & DXGI_FORMAT_B8G8R8X8_UNORM_SRGB should be
            // written using the DX10 extended header instead since these formats require
            // DXGI 1.1
            //
            // This code will use the fallback to swizzle BGR to RGB in memory for standard
            // DDS files which works on 10 and 10.1 devices with WDDM 1.0 drivers
            //
            // NOTE: We don't use DXGI_FORMAT_B8G8R8X8_UNORM or DXGI_FORMAT_B8G8R8X8_UNORM
            // here because they were defined for DXGI 1.0 but were not required for D3D10/10.1

            if( ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000) )
                return DXGI_FORMAT_R8G8B8A8_UNORM;

            // No D3DFMT_X8B8G8R8 in DXGI. We'll deal with it in a swizzle case to ensure
            // alpha channel is 255 (don't care formats could contain garbage)

            // Note that many common DDS reader/writers (including D3DX) swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assumme
            // below that the 'backwards' header mask is being used since it is most
            // likely written by D3DX. The more robust solution is to use the 'DX10'
            // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

            // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
            if( ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000) )
                return DXGI_FORMAT_R10G10B10A2_UNORM;

            // No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

            if( ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000) )
                return DXGI_FORMAT_R16G16_UNORM;

            if( ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000) )
                // Only 32-bit color channel format in D3D9 was R32F
                return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
            break;

        case 24:
            // No 24bpp DXGI formats
            break;

        case 16:
            // 5:5:5 & 5:6:5 formats are defined for DXGI, but are deprecated for D3D10, 10.x

            // No 4bpp, 3:3:2, 3:3:2:8, or paletted DXGI formats for 10.x
            break;
        }
    }
    else if( ddpf.flags & DDS_LUMINANCE )
    {
        switch (ddpf.RGBBitCount)
        {
        case 16:
            if (ISBITMASK(0xffff, 0, 0, 0))
                return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
            if (ISBITMASK(0x00ff, 0, 0, 0xff00))
                return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
            break;

        case 8:
            if (ISBITMASK(0xff, 0, 0, 0))
                return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension

            if (ISBITMASK(0x00ff, 0, 0, 0xff00))
                return DXGI_FORMAT_R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16

            // No 4bpp DXGI formats for 10.x
            break;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else if( ddpf.flags & DDS_ALPHA )
    {
        if( 8 == ddpf.RGBBitCount )
        {
            return DXGI_FORMAT_A8_UNORM;
        }
    }
    else if (ddpf.flags & DDS_BUMPDUDV)
    {
        switch (ddpf.RGBBitCount)
        {
        case 32:
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                return DXGI_FORMAT_R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
            if (ISBITMASK(0x0000ffff, 0xffff0000, 0, 0))
                return DXGI_FORMAT_R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension

            // No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
            break;

        case 16:
            if (ISBITMASK(0x00ff, 0xff00, 0, 0))
                return DXGI_FORMAT_R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
            break;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }

        // No DXGI format maps to DDPF_BUMPLUMINANCE aka D3DFMT_L6V5U5, D3DFMT_X8L8V8U8
    }
    else if( ddpf.flags & DDS_FOURCC )
    {
        if( MAKEFOURCC( 'D', 'X', 'T', '1' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC1_UNORM;
        if( MAKEFOURCC( 'D', 'X', 'T', '3' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC2_UNORM;
        if( MAKEFOURCC( 'D', 'X', 'T', '5' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC3_UNORM;

        // While pre-mulitplied alpha isn't directly supported by the DXGI formats,
        // they are basically the same as these BC formats so they can be mapped
        if( MAKEFOURCC( 'D', 'X', 'T', '2' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC2_UNORM;
        if( MAKEFOURCC( 'D', 'X', 'T', '4' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC3_UNORM;

        if( MAKEFOURCC( 'A', 'T', 'I', '1' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC4_UNORM;
        if( MAKEFOURCC( 'B', 'C', '4', 'U' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC4_UNORM;
        if( MAKEFOURCC( 'B', 'C', '4', 'S' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC4_SNORM;

        if( MAKEFOURCC( 'A', 'T', 'I', '2' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC5_UNORM;
        if( MAKEFOURCC( 'B', 'C', '5', 'U' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC5_UNORM;
        if( MAKEFOURCC( 'B', 'C', '5', 'S' ) == ddpf.fourCC )
            return DXGI_FORMAT_BC5_SNORM;

        if( MAKEFOURCC( 'R', 'G', 'B', 'G' ) == ddpf.fourCC )
            return DXGI_FORMAT_R8G8_B8G8_UNORM;
        if( MAKEFOURCC( 'G', 'R', 'G', 'B' ) == ddpf.fourCC )
            return DXGI_FORMAT_G8R8_G8B8_UNORM;

        // Check for D3DFORMAT enums being set here
        switch( ddpf.fourCC )
        {
        case D3DFMT_A16B16G16R16: // 36
            return DXGI_FORMAT_R16G16B16A16_UNORM;

        case D3DFMT_Q16W16V16U16: // 110
            return DXGI_FORMAT_R16G16B16A16_SNORM;

        case D3DFMT_R16F: // 111
            return DXGI_FORMAT_R16_FLOAT;

        case D3DFMT_G16R16F: // 112
            return DXGI_FORMAT_R16G16_FLOAT;

        case D3DFMT_A16B16G16R16F: // 113
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case D3DFMT_R32F: // 114
            return DXGI_FORMAT_R32_FLOAT;

        case D3DFMT_G32R32F: // 115
            return DXGI_FORMAT_R32G32_FLOAT;

        case D3DFMT_A32B32G32R32F: // 116
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

            // No DXGI format maps to D3DFMT_CxV8U8

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    return DXGI_FORMAT_UNKNOWN;
}


//--------------------------------------------------------------------------------------
static HRESULT CreateTextureFromDDS( LPDIRECT3DDEVICE9 pDev, DDS_HEADER* pHeader, __inout_bcount(BitSize) BYTE* pBitData, UINT BitSize,
                                     __out LPDIRECT3DBASETEXTURE9* ppTex )
{
    HRESULT hr = S_OK;

    UINT iWidth = pHeader->width;
    UINT iHeight = pHeader->height;

    UINT iMipCount = pHeader->mipMapCount;
    if( 0 == iMipCount )
        iMipCount = 1;

    // We could support a subset of 'DX10' extended header DDS files, but we'll assume here we are only
    // supporting legacy DDS files for a Direct3D9 device

    D3DFORMAT fmt = GetD3D9Format( pHeader->ddspf );
    if ( fmt == D3DFMT_UNKNOWN || BitsPerPixel( fmt ) == 0 )
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

    if ( pHeader->flags & DDS_HEADER_FLAGS_VOLUME )
    {
        UINT iDepth = pHeader->depth;

        // Create the volume texture (let the runtime do the validation)
        LPDIRECT3DVOLUMETEXTURE9 pTexture;
        LPDIRECT3DVOLUMETEXTURE9 pStagingTexture;
        hr = pDev->CreateVolumeTexture( iWidth, iHeight, iDepth, iMipCount,
                                        0, fmt, D3DPOOL_DEFAULT, &pTexture, NULL );
        if( FAILED( hr ) )
            return hr;

        hr = pDev->CreateVolumeTexture( iWidth, iHeight, iDepth, iMipCount,
                                        0, fmt, D3DPOOL_SYSTEMMEM, &pStagingTexture, NULL );
        if( FAILED( hr ) )
        {
            SAFE_RELEASE( pTexture );
            return hr;
        }

        // Lock, fill, unlock
        UINT NumBytes, RowBytes, NumRows;
        const BYTE* pSrcBits = pBitData;
        const BYTE *pEndBits = pBitData + BitSize;
        D3DLOCKED_BOX LockedBox = {};

        for( UINT i = 0; i < iMipCount; ++i )
        {
            GetSurfaceInfo( iWidth, iHeight, fmt, &NumBytes, &RowBytes, &NumRows );

            if ( ( pSrcBits + (NumBytes*iDepth) ) > pEndBits )
            {
                SAFE_RELEASE( pStagingTexture );
                SAFE_RELEASE( pTexture );
                return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
            }

            if( SUCCEEDED( pStagingTexture->LockBox( i, &LockedBox, NULL, 0 ) ) )
            {
                BYTE* pDestBits = ( BYTE* )LockedBox.pBits;

                for( UINT j = 0; j < iDepth; ++j )
                {
                    BYTE *dptr = pDestBits;
                    const BYTE *sptr = pSrcBits;

                    // Copy stride line by line
                    for( UINT h = 0; h < NumRows; h++ )
                    {
                        memcpy_s( dptr, LockedBox.RowPitch, sptr, RowBytes );
                        dptr += LockedBox.RowPitch;
                        sptr += RowBytes;
                    }

                    pDestBits += LockedBox.SlicePitch;
                    pSrcBits += NumBytes;
                }

                pStagingTexture->UnlockBox( i );
            }

            iWidth = iWidth >> 1;
            iHeight = iHeight >> 1;
            iDepth = iDepth >> 1;
            if( iWidth == 0 )
                iWidth = 1;
            if( iHeight == 0 )
                iHeight = 1;
            if( iDepth == 0 )
                iDepth = 1;
        }

        hr = pDev->UpdateTexture( pStagingTexture, pTexture );
        SAFE_RELEASE( pStagingTexture );
        if( FAILED( hr ) )
        {
            SAFE_RELEASE( pTexture );
            return hr;
        }

        *ppTex = pTexture;
    }
    else if ( pHeader->caps2 & DDS_CUBEMAP )
    {
        // We require at least one face to be defined, and the faces must be square
        if ( (pHeader->caps2 & DDS_CUBEMAP_ALLFACES ) == 0 || iHeight != iWidth )
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

        // Create the cubemap (let the runtime do the validation)
        LPDIRECT3DCUBETEXTURE9 pTexture;
        LPDIRECT3DCUBETEXTURE9 pStagingTexture;
        hr = pDev->CreateCubeTexture( iWidth, iMipCount,
                                      0, fmt, D3DPOOL_DEFAULT, &pTexture, NULL );
        if( FAILED( hr ) )
            return hr;

        hr = pDev->CreateCubeTexture( iWidth, iMipCount,
                                      0, fmt, D3DPOOL_SYSTEMMEM, &pStagingTexture, NULL );
        if( FAILED( hr ) )
        {
            SAFE_RELEASE( pTexture );
            return hr;
        }

        // Lock, fill, unlock
        UINT NumBytes, RowBytes, NumRows;
        const BYTE* pSrcBits = pBitData;
        const BYTE *pEndBits = pBitData + BitSize;
        D3DLOCKED_RECT LockedRect = {};

        UINT mask = DDS_CUBEMAP_POSITIVEX & ~DDS_CUBEMAP;
        for( UINT f = 0; f < 6; ++f, mask <<= 1 )
        {
            if( !(pHeader->caps2 & mask ) )
                continue;

            UINT w = iWidth;
            UINT h = iHeight;
            for( UINT i = 0; i < iMipCount; ++i )
            {
                GetSurfaceInfo( w, h, fmt, &NumBytes, &RowBytes, &NumRows );

                if ( (pSrcBits + NumBytes) > pEndBits )
                {
                    SAFE_RELEASE( pStagingTexture );
                    SAFE_RELEASE( pTexture );
                    return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
                }

                if( SUCCEEDED( pStagingTexture->LockRect( (D3DCUBEMAP_FACES)f, i, &LockedRect, NULL, 0 ) ) )
                {
                    BYTE* pDestBits = ( BYTE* )LockedRect.pBits;

                    // Copy stride line by line
                    for( UINT r = 0; r < NumRows; r++ )
                    {
                        memcpy_s( pDestBits, LockedRect.Pitch, pSrcBits, RowBytes );
                        pDestBits += LockedRect.Pitch;
                        pSrcBits += RowBytes;
                    }

                    pStagingTexture->UnlockRect( (D3DCUBEMAP_FACES)f, i );
                }

                w = w >> 1;
                h = h >> 1;
                if( w == 0 )
                    w = 1;
                if( h == 0 )
                    h = 1;
            }
        }

        hr = pDev->UpdateTexture( pStagingTexture, pTexture );
        SAFE_RELEASE( pStagingTexture );
        if( FAILED( hr ) )
        {
            SAFE_RELEASE( pTexture );
            return hr;
        }

        *ppTex = pTexture;
    }
    else
    {
        // Create the texture (let the runtime do the validation)
        LPDIRECT3DTEXTURE9 pTexture;
        LPDIRECT3DTEXTURE9 pStagingTexture;
        hr = pDev->CreateTexture( iWidth, iHeight, iMipCount,
                                  0, fmt, D3DPOOL_DEFAULT, &pTexture, NULL );
        if( FAILED( hr ) )
            return hr;

        hr = pDev->CreateTexture( iWidth, iHeight, iMipCount,
                                  0, fmt, D3DPOOL_SYSTEMMEM, &pStagingTexture, NULL );
        if( FAILED( hr ) )
        {
            SAFE_RELEASE( pTexture );
            return hr;
        }

        // Lock, fill, unlock
        UINT NumBytes, RowBytes, NumRows;
        const BYTE* pSrcBits = pBitData;
        const BYTE *pEndBits = pBitData + BitSize;
        D3DLOCKED_RECT LockedRect = {};

        for( UINT i = 0; i < iMipCount; ++i )
        {
            GetSurfaceInfo( iWidth, iHeight, fmt, &NumBytes, &RowBytes, &NumRows );

            if ( (pSrcBits + NumBytes) > pEndBits )
            {
                SAFE_RELEASE( pStagingTexture );
                SAFE_RELEASE( pTexture );
                return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
            }

            if( SUCCEEDED( pStagingTexture->LockRect( i, &LockedRect, NULL, 0 ) ) )
            {
                BYTE* pDestBits = ( BYTE* )LockedRect.pBits;

                // Copy stride line by line
                for( UINT h = 0; h < NumRows; h++ )
                {
                    memcpy_s( pDestBits, LockedRect.Pitch, pSrcBits, RowBytes );
                    pDestBits += LockedRect.Pitch;
                    pSrcBits += RowBytes;
                }

                pStagingTexture->UnlockRect( i );
            }

            iWidth = iWidth >> 1;
            iHeight = iHeight >> 1;
            if( iWidth == 0 )
                iWidth = 1;
            if( iHeight == 0 )
                iHeight = 1;
        }

        hr = pDev->UpdateTexture( pStagingTexture, pTexture );
        SAFE_RELEASE( pStagingTexture );
        if( FAILED( hr ) )
        {
            SAFE_RELEASE( pTexture );
            return hr;
        }

        *ppTex = pTexture;
    }

    return hr;
}


//--------------------------------------------------------------------------------------
static HRESULT CreateTextureFromDDS( ID3D10Device1* pDev, DDS_HEADER* pHeader, __inout_bcount(BitSize) BYTE* pBitData,
                                     UINT BitSize, __out ID3D10ShaderResourceView1** ppSRV, bool bSRGB )
{
    HRESULT hr = S_OK;

    UINT iWidth = pHeader->width;
    UINT iHeight = pHeader->height;
    UINT iDepth = pHeader->depth;

    UINT resDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    UINT arraySize = 1;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    bool isCubeMap = false;

    UINT iMipCount = pHeader->mipMapCount;
    if( 0 == iMipCount )
        iMipCount = 1;

    bool swaprgb = false;
    bool seta = false;

    if ((pHeader->ddspf.flags & DDS_FOURCC)
        && (MAKEFOURCC( 'D', 'X', '1', '0' ) == pHeader->ddspf.fourCC ) )
    {
        DDS_HEADER_DXT10* d3d10ext = (DDS_HEADER_DXT10*)( (char*)pHeader + sizeof(DDS_HEADER) );

        arraySize = d3d10ext->arraySize;
        if ( arraySize == 0 )
           return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        if ( BitsPerPixel( d3d10ext->dxgiFormat ) == 0 )
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

        format = d3d10ext->dxgiFormat;

        switch ( d3d10ext->resourceDimension )
        {
        case DDS_DIMENSION_TEXTURE1D:
            // D3DX writes 1D textures with a fixed Height of 1
            if ( (pHeader->flags & DDS_HEIGHT) && iHeight != 1 )
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            iHeight = iDepth = 1;
            break;

        case DDS_DIMENSION_TEXTURE2D:
            if ( d3d10ext->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE )
            {
                arraySize *= 6;
                isCubeMap = true;
            }
            iDepth = 1;
            break;

        case DDS_DIMENSION_TEXTURE3D:
            if ( !(pHeader->flags & DDS_HEADER_FLAGS_VOLUME) )
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

            if ( arraySize > 1 )
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            break;

        default:
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }

        resDim = d3d10ext->resourceDimension;
    }
    else
    {
        format = GetDXGIFormat( pHeader->ddspf );

        if ( pHeader->flags & DDS_HEADER_FLAGS_VOLUME )
        {
            resDim = DDS_DIMENSION_TEXTURE3D;
        }
        else
        {
            if ( pHeader->caps2 & DDS_CUBEMAP )
            {
               // We require all six faces to be defined
               if ( (pHeader->caps2 & DDS_CUBEMAP_ALLFACES ) != DDS_CUBEMAP_ALLFACES )
                   return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

                arraySize = 6;
                isCubeMap = true;
            }

            iDepth = 1;
            resDim = DDS_DIMENSION_TEXTURE2D;

            // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
        }

        if ( format == DXGI_FORMAT_UNKNOWN )
        {
            D3DFORMAT fmt = GetD3D9Format( pHeader->ddspf );

            // Swizzle some BGR to RGB common formats to be DXGI (1.0) supported
            // and/or set the alpha channel from 'don't care' to known value
            switch( fmt )
            {
            case D3DFMT_X8R8G8B8:
                format = DXGI_FORMAT_R8G8B8A8_UNORM;
                swaprgb = true;
                seta = true;
                break;

            case D3DFMT_A8R8G8B8:
                format = DXGI_FORMAT_R8G8B8A8_UNORM;
                swaprgb = true;
                break;

            case D3DFMT_X8B8G8R8:
                format = DXGI_FORMAT_R8G8B8A8_UNORM;
                seta = true;
                break;

            case D3DFMT_A2R10G10B10:
                format = DXGI_FORMAT_R10G10B10A2_UNORM;
                swaprgb = TRUE;
                break;

            // Would need temp memory to expand other formats (24bpp, 4bpp, 16-bit (5:6:5, 5:5:5, 5:5:5:1), 3:3:2, 3:3:2:8)

            default:
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            }
        }

        assert( BitsPerPixel( format ) != 0 );
    }

    // Bound sizes
    if ( iMipCount > D3D10_REQ_MIP_LEVELS )
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

    switch ( resDim )
    {
        case DDS_DIMENSION_TEXTURE1D:
            if ( (arraySize > D3D10_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION)
                 || (iWidth > D3D10_REQ_TEXTURE1D_U_DIMENSION) )
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            break;

        case DDS_DIMENSION_TEXTURE2D:
            if ( isCubeMap )
            {
                // This is the right bound because we set arraySize to (NumCubes*6) above
                if ( (arraySize > D3D10_REQ_TEXTURECUBE_DIMENSION)
                     || (iWidth > D3D10_REQ_TEXTURECUBE_DIMENSION)
                     || (iHeight > D3D10_REQ_TEXTURECUBE_DIMENSION))
                    return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            }
            else if ( (arraySize > D3D10_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION)
                 || (iWidth > D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION)
                 || (iHeight > D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION))
            {
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            }
            break;

        case DDS_DIMENSION_TEXTURE3D:
            if ( (arraySize > 1)
                 || (iWidth > D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
                 || (iHeight > D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
                 || (iDepth > D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) )
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            break;
        }


    if ( bSRGB )
        format = MAKE_SRGB( format );

    // Create the texture
    D3D10_SUBRESOURCE_DATA* pInitData = new (std::nothrow) D3D10_SUBRESOURCE_DATA[ iMipCount * arraySize ];
    if( !pInitData )
        return E_OUTOFMEMORY;

    UINT NumBytes = 0;
    UINT RowBytes = 0;
    UINT NumRows = 0;
    BYTE* pSrcBits = pBitData;
    const BYTE *pEndBits = pBitData + BitSize;

    UINT index = 0;
    for( UINT j = 0; j < arraySize; j++ )
    {
        UINT w = iWidth;
        UINT h = iHeight;
        UINT d = iDepth;
        for( UINT i = 0; i < iMipCount; i++ )
        {
            GetSurfaceInfo( w, h, format, &NumBytes, &RowBytes, &NumRows );

            pInitData[index].pSysMem = ( void* )pSrcBits;
            pInitData[index].SysMemPitch = RowBytes;
            pInitData[index].SysMemSlicePitch = NumBytes;
            ++index;

            if ( pSrcBits + (NumBytes*d) > pEndBits )
            {
                SAFE_DELETE_ARRAY( pInitData );
                return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
            }

            if ( swaprgb || seta )
            {
                switch( format )
                {
                case DXGI_FORMAT_R8G8B8A8_UNORM:
                    {
                        BYTE *sptr = pSrcBits;
                        for ( UINT slice = 0; slice < d; ++slice )
                        {
                            BYTE *rptr = sptr;
                            for ( UINT row = 0; row < NumRows; ++row )
                            {
                                BYTE *ptr = rptr;
                                for( UINT x = 0; x < w; ++x, ptr += 4 )
                                {
                                    if ( ptr + 4 <= pEndBits )
                                    {
                                        if ( swaprgb )
                                        {
                                            BYTE a = ptr[0];
                                            ptr[0] = ptr[2];
                                            ptr[2] = a;
                                        }
                                        if ( seta )
                                            ptr[3] = 255;
                                    }
                                }
                                rptr += RowBytes;
                            }
                            sptr += NumBytes;
                        }
                    }
                    break;

                case DXGI_FORMAT_R10G10B10A2_UNORM:
                    {
                        BYTE *sptr = pSrcBits;
                        for ( UINT slice = 0; slice < d; ++slice )
                        {
                            const BYTE *rptr = sptr;
                            for ( UINT row = 0; row < NumRows; ++row )
                            {
                                DWORD *ptr = (DWORD*)rptr;
                                for( UINT x = 0; x < w; ++x, ++ptr )
                                {
                                    if ( ptr + 1 <= (DWORD*)pEndBits )
                                    {
                                        DWORD t = *ptr;
                                        DWORD u = (t & 0x3ff00000) >> 20;
                                        DWORD v = (t & 0x000003ff) << 20;
                                        *ptr = ( t & ~0x3ff003ff ) | u | v;
                                    }
                                }
                                rptr += RowBytes;
                            }
                            sptr += NumBytes;
                        }
                    }
                    break;
                }
            }

            pSrcBits += NumBytes * d;

            w = w >> 1;
            h = h >> 1;
            d = d >> 1;
            if( w == 0 )
                w = 1;
            if( h == 0 )
                h = 1;
            if( d == 0 )
                d = 1;
        }
    }

    switch ( resDim )
    {
        case DDS_DIMENSION_TEXTURE1D:
            {
                D3D10_TEXTURE1D_DESC desc;
                desc.Width = iWidth;
                desc.MipLevels = iMipCount;
                desc.ArraySize = arraySize;
                desc.Format = format;
                desc.Usage = D3D10_USAGE_DEFAULT;
                desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.MiscFlags = 0;

                ID3D10Texture1D* pTex = NULL;
                hr = pDev->CreateTexture1D( &desc, pInitData, &pTex );
                if( SUCCEEDED( hr ) && pTex )
                {
#if defined(DEBUG) || defined(PROFILE)
                    pTex->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof("DDSTextureLoader")-1, "DDSTextureLoader" );
#endif
                    D3D10_SHADER_RESOURCE_VIEW_DESC1 SRVDesc;
                    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
                    SRVDesc.Format = format;

                    if ( arraySize > 1 )
                    {
                        SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1DARRAY;
                        SRVDesc.Texture1DArray.MipLevels = desc.MipLevels;
                        SRVDesc.Texture1DArray.ArraySize = arraySize;
                    }
                    else
                    {
                        SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1D;
                        SRVDesc.Texture1D.MipLevels = desc.MipLevels;
                    }

                    hr = pDev->CreateShaderResourceView1( pTex, &SRVDesc, ppSRV );
                    SAFE_RELEASE( pTex );
                }
            }
           break;

        case DDS_DIMENSION_TEXTURE2D:
            {
                D3D10_TEXTURE2D_DESC desc;
                desc.Width = iWidth;
                desc.Height = iHeight;
                desc.MipLevels = iMipCount;
                desc.ArraySize = arraySize;
                desc.Format = format;
                desc.SampleDesc.Count = 1;
                desc.SampleDesc.Quality = 0;
                desc.Usage = D3D10_USAGE_DEFAULT;
                desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.MiscFlags = (isCubeMap) ? D3D10_RESOURCE_MISC_TEXTURECUBE : 0;

                ID3D10Texture2D* pTex = NULL;
                hr = pDev->CreateTexture2D( &desc, pInitData, &pTex );
                if( SUCCEEDED( hr ) && pTex )
                {
#if defined(DEBUG) || defined(PROFILE)
                    pTex->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof("DDSTextureLoader")-1, "DDSTextureLoader" );
#endif
                    D3D10_SHADER_RESOURCE_VIEW_DESC1 SRVDesc;
                    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
                    SRVDesc.Format = format;

                    if ( isCubeMap )
                    {
                        if ( arraySize > 6 )
                        {
                            SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
                            SRVDesc.TextureCubeArray.MipLevels = desc.MipLevels;

                            // Earlier we set arraySize to (NumCubes * 6)
                            SRVDesc.TextureCubeArray.NumCubes = arraySize / 6;
                        }
                        else
                        {
                            SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
                            SRVDesc.TextureCube.MipLevels = desc.MipLevels;
                        }
                    }
                    else if ( arraySize > 1 )
                    {
                        SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
                        SRVDesc.Texture2DArray.MipLevels = desc.MipLevels;
                        SRVDesc.Texture2DArray.ArraySize = arraySize;
                    }
                    else
                    {
                        SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
                        SRVDesc.Texture2D.MipLevels = desc.MipLevels;
                    }

                    hr = pDev->CreateShaderResourceView1(pTex, &SRVDesc, ppSRV);
                    SAFE_RELEASE( pTex );
                }
            }
            break;

        case DDS_DIMENSION_TEXTURE3D:
            {
                D3D10_TEXTURE3D_DESC desc;
                desc.Width = iWidth;
                desc.Height = iHeight;
                desc.Depth = iDepth;
                desc.MipLevels = iMipCount;
                desc.Format = format;
                desc.Usage = D3D10_USAGE_DEFAULT;
                desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.MiscFlags = 0;

                ID3D10Texture3D* pTex = NULL;
                hr = pDev->CreateTexture3D( &desc, pInitData, &pTex );
                if( SUCCEEDED( hr ) && pTex )
                {
#if defined(DEBUG) || defined(PROFILE)
                    pTex->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof("DDSTextureLoader")-1, "DDSTextureLoader" );
#endif
                    D3D10_SHADER_RESOURCE_VIEW_DESC1 SRVDesc;
                    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
                    SRVDesc.Format = format;
                    SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE3D;
                    SRVDesc.Texture3D.MipLevels = desc.MipLevels;

                    hr = pDev->CreateShaderResourceView1( pTex, &SRVDesc, ppSRV );
                    SAFE_RELEASE( pTex );
                }
            }
            break;
    }

    SAFE_DELETE_ARRAY( pInitData );

    return hr;
}

//--------------------------------------------------------------------------------------
HRESULT CreateDDSTextureFromFile( LPDIRECT3DDEVICE9 pDev, const WCHAR* szFileName, LPDIRECT3DBASETEXTURE9* ppTex )
{
    if ( !pDev || !szFileName || !ppTex )
        return E_INVALIDARG;

    BYTE* pHeapData = NULL;
    DDS_HEADER* pHeader= NULL;
    BYTE* pBitData = NULL;
    UINT BitSize = 0;

    HRESULT hr = LoadTextureDataFromFile( szFileName, &pHeapData, &pHeader, &pBitData, &BitSize );
    if(FAILED(hr))
    {
        SAFE_DELETE_ARRAY( pHeapData );
        return hr;
    }

    hr = CreateTextureFromDDS( pDev, pHeader, pBitData, BitSize, ppTex );
    SAFE_DELETE_ARRAY( pHeapData );
    return hr;
}

HRESULT CreateDDSTextureFromFile( __in LPDIRECT3DDEVICE9 pDev, __in_z const WCHAR* szFileName, __out_opt LPDIRECT3DTEXTURE9* ppTex )
{
    if ( !pDev || !szFileName || !ppTex )
        return E_INVALIDARG;

    IDirect3DBaseTexture9* pTex = 0;
    HRESULT hr = CreateDDSTextureFromFile( pDev, szFileName, &pTex );
    if ( SUCCEEDED(hr) )
    {
        if ( pTex->GetType() == D3DRTYPE_TEXTURE )
        {
            *ppTex = (LPDIRECT3DTEXTURE9)pTex;
            return S_OK;
        }
        else
            hr = E_FAIL;
    }
    SAFE_RELEASE( pTex );
    return hr;
}

HRESULT CreateDDSTextureFromFile( __in LPDIRECT3DDEVICE9 pDev, __in_z const WCHAR* szFileName, __out_opt LPDIRECT3DCUBETEXTURE9* ppTex )
{
     if ( !pDev || !szFileName || !ppTex )
        return E_INVALIDARG;

    IDirect3DBaseTexture9* pTex = 0;
    HRESULT hr = CreateDDSTextureFromFile( pDev, szFileName, &pTex );
    if ( SUCCEEDED(hr) )
    {
        if ( pTex->GetType() == D3DRTYPE_CUBETEXTURE )
        {
            *ppTex = (LPDIRECT3DCUBETEXTURE9)pTex;
            return S_OK;
        }
        else
            hr = E_FAIL;
    }
    SAFE_RELEASE( pTex );
    return hr;
}

HRESULT CreateDDSTextureFromFile( __in LPDIRECT3DDEVICE9 pDev, __in_z const WCHAR* szFileName, __out_opt LPDIRECT3DVOLUMETEXTURE9* ppTex )
{
    if ( !pDev || !szFileName || !ppTex )
        return E_INVALIDARG;

    IDirect3DBaseTexture9* pTex = 0;
    HRESULT hr = CreateDDSTextureFromFile( pDev, szFileName, &pTex );
    if ( SUCCEEDED(hr) )
    {
        if ( pTex->GetType() == D3DRTYPE_VOLUMETEXTURE )
        {
            *ppTex = (LPDIRECT3DVOLUMETEXTURE9)pTex;
            return S_OK;
        }
        else
            hr = E_FAIL;
    }
    SAFE_RELEASE( pTex );
    return hr;
}

//--------------------------------------------------------------------------------------
HRESULT CreateDDSTextureFromFile( ID3D10Device1* pDev, const WCHAR* szFileName, ID3D10ShaderResourceView1** ppSRV, bool bSRGB )
{
    if ( !pDev || !szFileName || !ppSRV )
        return E_INVALIDARG;

    BYTE* pHeapData = NULL;
    DDS_HEADER* pHeader = NULL;
    BYTE* pBitData = NULL;
    UINT BitSize = 0;

    HRESULT hr = LoadTextureDataFromFile( szFileName, &pHeapData, &pHeader, &pBitData, &BitSize );
    if(FAILED(hr))
    {
        SAFE_DELETE_ARRAY( pHeapData );
        return hr;
    }

    hr = CreateTextureFromDDS( pDev, pHeader, pBitData, BitSize, ppSRV, bSRGB );
    SAFE_DELETE_ARRAY( pHeapData );

#if defined(DEBUG) || defined(PROFILE)
    if ( *ppSRV )
    {
        CHAR strFileA[MAX_PATH];
        WideCharToMultiByte( CP_ACP, 0, szFileName, -1, strFileA, MAX_PATH, NULL, FALSE );
        CHAR* pstrName = strrchr( strFileA, '\\' );
        if( pstrName == NULL )
            pstrName = strFileA;
        else
            pstrName++;

        (*ppSRV)->SetPrivateData( WKPDID_D3DDebugObjectName, lstrlenA(pstrName), pstrName );
    }
#endif

    return hr;
}
