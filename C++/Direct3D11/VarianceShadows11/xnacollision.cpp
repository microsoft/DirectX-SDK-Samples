//-------------------------------------------------------------------------------------
// XNACollision.cpp
//  
// An opimtized collision library based on XNAMath
//  
// Microsoft XNA Developer Connection
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "DXUT.h"
#include "xnacollision.h"

namespace XNA
{


//-----------------------------------------------------------------------------
// Build a frustum from a persepective projection matrix.  The matrix may only
// contain a projection; any rotation, translation or scale will cause the
// constructed frustum to be incorrect.
//-----------------------------------------------------------------------------
VOID ComputeFrustumFromProjection( Frustum* pOut, XMMATRIX* pProjection )
{
    XMASSERT( pOut );
    XMASSERT( pProjection );

    // Corners of the projection frustum in homogenous space.
    static XMVECTOR HomogenousPoints[6] =
    {
        {  1.0f,  0.0f, 1.0f, 1.0f },   // right (at far plane)
        { -1.0f,  0.0f, 1.0f, 1.0f },   // left
        {  0.0f,  1.0f, 1.0f, 1.0f },   // top
        {  0.0f, -1.0f, 1.0f, 1.0f },   // bottom

        { 0.0f, 0.0f, 0.0f, 1.0f },     // near
        { 0.0f, 0.0f, 1.0f, 1.0f }      // far
    };

    XMVECTOR Determinant;
    XMMATRIX matInverse = XMMatrixInverse( &Determinant, *pProjection );

    // Compute the frustum corners in world space.
    XMVECTOR Points[6];

    for( INT i = 0; i < 6; i++ )
    {
        // Transform point.
        Points[i] = XMVector4Transform( HomogenousPoints[i], matInverse );
    }

    pOut->Origin = XMFLOAT3( 0.0f, 0.0f, 0.0f );
    pOut->Orientation = XMFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f );

    // Compute the slopes.
    Points[0] = Points[0] * XMVectorReciprocal( XMVectorSplatZ( Points[0] ) );
    Points[1] = Points[1] * XMVectorReciprocal( XMVectorSplatZ( Points[1] ) );
    Points[2] = Points[2] * XMVectorReciprocal( XMVectorSplatZ( Points[2] ) );
    Points[3] = Points[3] * XMVectorReciprocal( XMVectorSplatZ( Points[3] ) );

    pOut->RightSlope = XMVectorGetX( Points[0] );
    pOut->LeftSlope = XMVectorGetX( Points[1] );
    pOut->TopSlope = XMVectorGetY( Points[2] );
    pOut->BottomSlope = XMVectorGetY( Points[3] );

    // Compute near and far.
    Points[4] = Points[4] * XMVectorReciprocal( XMVectorSplatW( Points[4] ) );
    Points[5] = Points[5] * XMVectorReciprocal( XMVectorSplatW( Points[5] ) );

    pOut->Near = XMVectorGetZ( Points[4] );
    pOut->Far = XMVectorGetZ( Points[5] );

    return;
}



}; // namespace
