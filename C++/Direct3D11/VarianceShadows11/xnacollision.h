//-------------------------------------------------------------------------------------
// XNACollision.h
//  
// An opimtized collision library based on XNAMath
//  
// Microsoft XNA Developer Connection
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#ifndef _XNA_COLLISION_H_
#define _XNA_COLLISION_H_

#include <xnamath.h>

namespace XNA
{

//-----------------------------------------------------------------------------
// Bounding volumes structures.
//
// The bounding volume structures are setup for near minimum size because there
// are likely to be many of them, and memory bandwidth and space will be at a
// premium relative to CPU cycles on Xbox 360.
//-----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable: 4324)

_DECLSPEC_ALIGN_16_ struct Frustum
{
    XMFLOAT3 Origin;            // Origin of the frustum (and projection).
    XMFLOAT4 Orientation;       // Unit quaternion representing rotation.

    FLOAT RightSlope;           // Positive X slope (X/Z).
    FLOAT LeftSlope;            // Negative X slope.
    FLOAT TopSlope;             // Positive Y slope (Y/Z).
    FLOAT BottomSlope;          // Negative Y slope.
    FLOAT Near, Far;            // Z of the near plane and far plane.
};

#pragma warning(pop)

//-----------------------------------------------------------------------------
// Bounding volume construction.
//-----------------------------------------------------------------------------
VOID ComputeFrustumFromProjection( Frustum* pOut, XMMATRIX* pProjection );

}; // namespace

#endif