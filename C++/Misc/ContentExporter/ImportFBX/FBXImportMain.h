//-------------------------------------------------------------------------------------
//  FBXImportMain.h
//
//  Entry points for FBX scene parsing.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

using namespace ATG;

class FBXTransformer : public IDCCTransformer
{
public:
	FBXTransformer()
		: m_fUnitScale( 1.0f )
	{ }

    VOID Initialize( KFbxScene* pScene );

    virtual VOID TransformMatrix( D3DXMATRIX* pDestMatrix, CONST D3DXMATRIX* pSrcMatrix ) CONST;
    virtual VOID TransformPosition( D3DXVECTOR3* pDestPosition, CONST D3DXVECTOR3* pSrcPosition ) CONST;
    virtual VOID TransformDirection( D3DXVECTOR3* pDestDirection, CONST D3DXVECTOR3* pSrcDirection ) CONST;
    virtual FLOAT TransformLength( FLOAT fInputLength ) CONST;

protected:
	FLOAT m_fUnitScale;
    BOOL  m_bMaxConversion;
};

class FBXImport
{
public:
    static HRESULT Initialize();
    static VOID ClearScene();

    static HRESULT ImportFile( const CHAR* strFileName );

private:
};
