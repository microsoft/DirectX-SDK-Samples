//-------------------------------------------------------------------------------------
//  collision.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "collision.h"

namespace ATG
{

//-----------------------------------------------------------------------------
// Find the approximate smallest enclosing bounding sphere for a set of 
// points. Exact computation of the smallest enclosing bounding sphere is 
// possible but is slower and requires a more complex algorithm.
// The algorithm is based on  Jack Ritter, "An Efficient Bounding Sphere", 
// Graphics Gems.
//-----------------------------------------------------------------------------
VOID ComputeBoundingSphereFromPoints( Sphere* pOut, UINT Count, const XMFLOAT3* pPoints, UINT Stride )
{
    XMASSERT( pOut );
    XMASSERT( Count > 0 );
    XMASSERT( pPoints );

    // Find the points with minimum and maximum x, y, and z
    XMVECTOR MinX, MaxX, MinY, MaxY, MinZ, MaxZ;
    
    MinX = MaxX = MinY = MaxY = MinZ = MaxZ = XMLoadFloat3(pPoints);
    
    for (UINT i = 1; i < Count; i++)
    {
        XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)pPoints + i*Stride));
        
        if (XMVectorGetX(Point) < XMVectorGetX(MinX))
            MinX = Point;

        if (XMVectorGetX(Point) > XMVectorGetX(MaxX))
            MaxX = Point;
        
        if (XMVectorGetY(Point) < XMVectorGetY(MinY))
            MinY = Point;

        if (XMVectorGetY(Point) > XMVectorGetY(MaxY))
            MaxY = Point;

        if (XMVectorGetZ(Point) < XMVectorGetZ(MinZ))
            MinZ = Point;

        if (XMVectorGetZ(Point) > XMVectorGetZ(MaxZ))
            MaxZ = Point;
    }
    
    // Use the min/max pair that are farthest apart to form the initial sphere.
    XMVECTOR DeltaX = MaxX - MinX;
    XMVECTOR DistX = XMVector3Length(DeltaX);

    XMVECTOR DeltaY = MaxY - MinY;
    XMVECTOR DistY = XMVector3Length(DeltaY);

    XMVECTOR DeltaZ = MaxZ - MinZ;
    XMVECTOR DistZ = XMVector3Length(DeltaZ);
    
    XMVECTOR Center;
    XMVECTOR Radius;

    if (XMVector3Greater(DistX, DistY))
    {
        if (XMVector3Greater(DistX, DistZ))
        {
            // Use min/max x.
            Center = (MaxX + MinX) * 0.5f;
            Radius = DistX * 0.5f;
        }
        else
        {
            // Use min/max z.
            Center = (MaxZ + MinZ) * 0.5f;
            Radius = DistZ * 0.5f;
        }
    }
    else // Y >= X
    {
        if (XMVector3Greater(DistY, DistZ))
        {
            // Use min/max y.
            Center = (MaxY + MinY) * 0.5f;
            Radius = DistY * 0.5f;
        }
        else
        {
            // Use min/max z.
            Center = (MaxZ + MinZ) * 0.5f;
            Radius = DistZ * 0.5f;
        }
    }
    
    // Add any points not inside the sphere.
    for (UINT i = 0; i < Count; i++)
    {
        XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)pPoints + i*Stride));

        XMVECTOR Delta = Point - Center;

        XMVECTOR Dist = XMVector3Length(Delta);
        
        if (XMVector3Greater(Dist, Radius))
        {
            // Adjust sphere to include the new point.
            Radius = (Radius + Dist) * 0.5f;
            Center += (XMVectorReplicate(1.0f) - Radius * XMVectorReciprocal(Dist)) * Delta;
        }
    }
    
    XMStoreFloat3(&pOut->Center, Center);
    pOut->Radius = XMVectorGetX(Radius);
    
    return;
}



//-----------------------------------------------------------------------------
// Find the minimum axis aligned bounding box containing a set of points.
//-----------------------------------------------------------------------------
VOID ComputeBoundingAxisAlignedBoxFromPoints( AxisAlignedBox* pOut, UINT Count, const XMFLOAT3* pPoints, UINT Stride )
{
    XMASSERT( pOut );
    XMASSERT( Count > 0 );
    XMASSERT( pPoints );

    // Find the minimum and maximum x, y, and z
    XMVECTOR vMin, vMax;
    
    vMin = vMax = XMLoadFloat3(pPoints);
    
    for (UINT i = 1; i < Count; i++)
    {
        XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)pPoints + i*Stride));
 
        vMin = XMVectorMin( vMin, Point );
        vMax = XMVectorMax( vMax, Point );
    }

    // Store center and extents.
    XMStoreFloat3( &pOut->Center,  (vMin + vMax) * 0.5f  );
    XMStoreFloat3( &pOut->Extents, (vMax - vMin) * 0.5f );
    
    return;
}



//-----------------------------------------------------------------------------
static inline BOOL SolveCubic( FLOAT e, FLOAT f, FLOAT g, FLOAT *t, FLOAT *u, FLOAT *v )
{
    FLOAT p, q, h, rc, d, theta, costh3, sinth3;

    p = f - e * e / 3.0f;
    q = g - e * f / 3.0f + e * e * e * 2.0f / 27.0f;
    h = q * q / 4.0f + p * p * p / 27.0f;
    
    if (h > 0.0) 
    {
        return FALSE; // only one real root
    }

    if ((h == 0.0) && (q == 0.0)) // all the same root
    {
        *t = - e / 3;
        *u = - e / 3;
        *v = - e / 3;
        
        return TRUE;
    }

    d = sqrtf(q * q / 4.0f - h);
    if (d < 0)
        rc = -powf(-d, 1.0f/3.0f);
    else
        rc = powf(d, 1.0f/3.0f);
    
    theta = acosf(-q / (2.0f * d));
    costh3 = cosf(theta / 3.0f);
    sinth3 = sqrtf(3.0f) * sinf(theta / 3.0f);
    *t = 2.0f * rc * costh3 - e / 3.0f;  
    *u = -rc * (costh3 + sinth3) - e / 3.0f;
    *v = -rc * (costh3 - sinth3) - e / 3.0f;
    
    return TRUE;
}



//-----------------------------------------------------------------------------
static inline XMVECTOR CalculateEigenVector( FLOAT m11, FLOAT m12, FLOAT m13, 
                                             FLOAT m22, FLOAT m23, FLOAT m33, FLOAT e )
{
    XMVECTOR vTmp = XMVectorZero();
    FLOAT f1, f2, f3;

    vTmp = XMVectorSetX( vTmp, (FLOAT)(m12 * m23 - m13 * (m22 - e)) );
    vTmp = XMVectorSetY( vTmp, (FLOAT)(m13 * m12 - m23 * (m11 - e)) );
    vTmp = XMVectorSetZ( vTmp, (FLOAT)((m11 - e) * (m22 - e) - m12 * m12) );

    if ((XMVectorGetX(vTmp) == 0.0) && (XMVectorGetY(vTmp) == 0.0) && (XMVectorGetZ(vTmp) == 0.0)) // planar or linear
    {
        // we only have one equation - find a valid one
        if ((m11 - e != 0.0) || (m12 != 0.0) || (m13 != 0.0))
        {
            f1 = m11 - e; f2 = m12; f3 = m13;
        }
        else if ((m12 != 0.0) || (m22 - e != 0.0) || (m23 != 0.0))
        {
            f1 = m12; f2 = m22 - e; f3 = m23;   
        }
        else if ((m13 != 0.0) || (m23 != 0.0) || (m33 - e != 0.0))
        {
            f1 = m13; f2 = m23; f3 = m33 - e;
        }
        else
        {
            // error, we'll just make something up - we have NO context
            f1 = 1.0; f2 = 0.0; f3 = 0.0;
        }
    
        if (f1 == 0.0) 
            vTmp = XMVectorSetX( vTmp, 0.0f ); 
        else
            vTmp = XMVectorSetX( vTmp, 1.0f );

        if (f2 == 0.0)
            vTmp = XMVectorSetY( vTmp, 0.0f );
        else
            vTmp = XMVectorSetY( vTmp, 1.0f );

        if (f3 == 0.0)
        {   
            vTmp = XMVectorSetZ( vTmp, 0.0f );
            // recalculate y to make equation work
            if (m12 != 0.0)
                vTmp = XMVectorSetY( vTmp, (FLOAT)(-f1 / f2) );
        }
        else
        {
            vTmp = XMVectorSetZ( vTmp, (FLOAT)((f2 - f1) / f3) );
        }
    }
    
    if (XMVectorGetX( XMVector3LengthSq( vTmp ) ) > 1e-5f)
    {
        return XMVector3Normalize( vTmp );
    }
    else
    {
        // Multiply by a value large enough to make the vector non-zero.
        vTmp *= 1e5f;
        return XMVector3Normalize( vTmp );
    }
}



//-----------------------------------------------------------------------------
static inline BOOL CalculateEigenVectors( FLOAT m11, FLOAT m12, FLOAT m13, 
                                          FLOAT m22, FLOAT m23, FLOAT m33, 
                                          FLOAT e1, FLOAT e2, FLOAT e3, 
                                          XMVECTOR *pV1, XMVECTOR *pV2, XMVECTOR *pV3 )
{
    XMVECTOR vTmp, vUp, vRight;

    BOOL v1z, v2z, v3z, e12, e13, e23;

    vUp = XMVectorSet( 0, 1, 0, 0 );
    vRight = XMVectorSet( 1, 0, 0, 0 );

    *pV1 = CalculateEigenVector( m11, m12, m13, m22, m23, m33, e1 );
    *pV2 = CalculateEigenVector( m11, m12, m13, m22, m23, m33, e2 );
    *pV3 = CalculateEigenVector( m11, m12, m13, m22, m23, m33, e3 );

    v1z = v2z = v3z = FALSE;

    if ((XMVectorGetX(*pV1) == 0.0) && (XMVectorGetY(*pV1) == 0.0) && (XMVectorGetZ(*pV1) == 0.0)) v1z = TRUE;
    if ((XMVectorGetX(*pV2) == 0.0) && (XMVectorGetY(*pV2) == 0.0) && (XMVectorGetZ(*pV2) == 0.0)) v2z = TRUE;
    if ((XMVectorGetX(*pV3) == 0.0) && (XMVectorGetY(*pV3) == 0.0) && (XMVectorGetZ(*pV3) == 0.0)) v3z = TRUE;

    e12 = (fabsf(XMVectorGetX(XMVector3Dot( *pV1, *pV2 ))) >  0.1f); // check for non-orthogonal vectors
    e13 = (fabsf(XMVectorGetX(XMVector3Dot( *pV1, *pV3 ))) >  0.1f);
    e23 = (fabsf(XMVectorGetX(XMVector3Dot( *pV2, *pV3 ))) >  0.1f);

    if ((v1z && v2z && v3z) || (e12 && e13 && e23) ||
        (e12 && v3z) || (e13 && v2z) || (e23 && v1z)) // all eigenvectors are 0- any basis set
    {
        *pV1 = XMVectorSet( 1, 0, 0, 0 );
        *pV2 = XMVectorSet( 0, 1, 0, 0 );
        *pV3 = XMVectorSet( 0, 0, 1, 0 );
        return TRUE;
    }

    if (v1z && v2z)
    {
        vTmp = XMVector3Cross( vUp, *pV3 );
        if ( XMVectorGetX(XMVector3LengthSq( vTmp )) < 1e-5f )
        {
            vTmp = XMVector3Cross( vRight, *pV3 );
        }
        *pV1 = XMVector3Normalize( vTmp );
        *pV2 = XMVector3Cross( *pV3, *pV1 );
        return TRUE;
    }

    if (v3z && v1z)
    {
        vTmp = XMVector3Cross( vUp, *pV2 );
        if ( XMVectorGetX(XMVector3LengthSq( vTmp )) < 1e-5f )
        {
            vTmp = XMVector3Cross( vRight, *pV2 );
        }
        *pV3 = XMVector3Normalize( vTmp );
        *pV1 = XMVector3Cross( *pV2, *pV3 );
        return TRUE;
    }

    if (v2z && v3z)
    {
        vTmp = XMVector3Cross( vUp, *pV1 );
        if ( XMVectorGetX(XMVector3LengthSq( vTmp )) < 1e-5f )
        {
            vTmp = XMVector3Cross( vRight, *pV1 );
        }
        *pV2 = XMVector3Normalize( vTmp );
        *pV3 = XMVector3Cross( *pV1, *pV2 );
        return TRUE;
    }

    if ((v1z) || e12)
    {   
        *pV1 = XMVector3Cross( *pV2, *pV3 );
        return TRUE;
    }

    if ((v2z) || e23)
    {   
        *pV2 = XMVector3Cross( *pV3, *pV1 );
        return TRUE;
    }

    if ((v3z) || e13)
    {   
        *pV3 = XMVector3Cross( *pV1, *pV2 );
        return TRUE;
    }
    
    return TRUE;
}



//-----------------------------------------------------------------------------
static inline BOOL CalculateEigenVectorsFromCovarianceMatrix( FLOAT Cxx, FLOAT Cyy, FLOAT Czz, 
                                                              FLOAT Cxy, FLOAT Cxz, FLOAT Cyz, 
                                                              XMVECTOR *pV1, XMVECTOR *pV2, XMVECTOR *pV3 )
{
    FLOAT e, f, g, ev1, ev2, ev3;

    // Calculate the eigenvalues by solving a cubic equation.
    e = -(Cxx + Cyy + Czz);
    f = Cxx * Cyy + Cyy * Czz + Czz * Cxx - Cxy * Cxy - Cxz * Cxz - Cyz * Cyz;
    g = Cxy * Cxy * Czz + Cxz * Cxz * Cyy + Cyz * Cyz * Cxx - Cxy * Cyz * Cxz * 2.0f - Cxx * Cyy * Czz;

    if ( !SolveCubic( e, f, g, &ev1, &ev2, &ev3 ) ) 
    {
        // set them to arbitrary orthonormal basis set
        *pV1 = XMVectorSet( 1, 0, 0, 0 );
        *pV2 = XMVectorSet( 0, 1, 0, 0 );
        *pV3 = XMVectorSet( 0, 0, 1, 0 );
        return FALSE;
    }
    
    return CalculateEigenVectors( Cxx, Cxy, Cxz, Cyy, Cyz, Czz, ev1, ev2, ev3, pV1, pV2, pV3 );
}



//-----------------------------------------------------------------------------
// Find the approximate minimum oriented bounding box containing a set of 
// points.  Exact computation of minimum oriented bounding box is possible but 
// is slower and requires a more complex algorithm.
// The algorithm works by computing the inertia tensor of the points and then
// using the eigenvectors of the intertia tensor as the axes of the box.
// Computing the intertia tensor of the convex hull of the points will usually 
// result in better bounding box but the computation is more complex. 
// Exact computation of the minimum oriented bounding box is possible but the
// best know algorithm is O(N^3) and is significanly more complex to implement.
//-----------------------------------------------------------------------------
VOID ComputeBoundingOrientedBoxFromPoints( OrientedBox* pOut, UINT Count, const XMFLOAT3* pPoints, UINT Stride )
{
    static CONST XMVECTORI32 PermuteXXY = { XM_PERMUTE_0X, XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0W };
    static CONST XMVECTORI32 PermuteYZZ = { XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_0Z, XM_PERMUTE_0W };

    XMASSERT( pOut );
    XMASSERT( Count > 0 );
    XMASSERT( pPoints );
    
    XMVECTOR CenterOfMass = XMVectorZero();
    
    // Compute the center of mass and inertia tensor of the points.
    for (UINT i = 0; i < Count; i++)
    {
        XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)pPoints + i*Stride));
        
        CenterOfMass += Point;
    }
    
    CenterOfMass *= XMVectorReciprocal( XMVectorReplicate( FLOAT(Count) ) );

    // Compute the inertia tensor of the points around the center of mass.
    // Using the center of mass is not strictly necessary, but will hopefully
    // improve the stability of finding the eigenvectors.
    XMVECTOR XX_YY_ZZ = XMVectorZero();
    XMVECTOR XY_XZ_YZ = XMVectorZero();

    for (UINT i = 0; i < Count; i++)
    {
        XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)pPoints + i*Stride)) - CenterOfMass;
        
        XX_YY_ZZ += Point * Point;
        
        XMVECTOR XXY = XMVectorPermute( Point, Point, PermuteXXY );
        XMVECTOR YZZ = XMVectorPermute( Point, Point, PermuteYZZ );
        
        XY_XZ_YZ += XXY * YZZ;
    }
    
    XMVECTOR v1, v2, v3;
    
    // Compute the eigenvectors of the inertia tensor.
    CalculateEigenVectorsFromCovarianceMatrix( XMVectorGetX(XX_YY_ZZ), XMVectorGetY(XX_YY_ZZ), XMVectorGetZ(XX_YY_ZZ),
                                               XMVectorGetX(XY_XZ_YZ), XMVectorGetY(XY_XZ_YZ), XMVectorGetZ(XY_XZ_YZ),
                                               &v1, &v2, &v3 );
    
    // Put them in a matrix.
    XMMATRIX R;
    
    R.r[0] = v1;    R.r[0] = XMVectorSetW( R.r[0], 0.0f );
    R.r[1] = v2;    R.r[1] = XMVectorSetW( R.r[1], 0.0f );
    R.r[2] = v3;    R.r[2] = XMVectorSetW( R.r[2], 0.0f );
    R.r[3] = XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f );

    // Multiply by -1 to convert the matrix into a right handed coordinate 
    // system (Det ~= 1) in case the eigenvectors form a left handed 
    // coordinate system (Det ~= -1) because XMQuaternionRotationMatrix only 
    // works on right handed matrices.
    XMVECTOR Det = XMMatrixDeterminant( R );
  
    if ( XMVector4Less( Det, XMVectorZero() ) )
    {
        const XMVECTOR VectorNegativeOne = { -1.0f, -1.0f, -1.0f, -1.0f };
        
        R.r[0] *= VectorNegativeOne;
        R.r[1] *= VectorNegativeOne;
        R.r[2] *= VectorNegativeOne;
    }
    
    // Get the rotation quaternion from the matrix.
    XMVECTOR Orientation = XMQuaternionRotationMatrix( R );
    
    // Make sure it is normal (in case the vectors are slightly non-orthogonal).
    Orientation = XMQuaternionNormalize( Orientation );

    // Rebuild the rotation matrix from the quaternion.
    R = XMMatrixRotationQuaternion( Orientation );

    // Build the rotation into the rotated space.
    XMMATRIX InverseR = XMMatrixTranspose( R );
    
    // Find the minimum OBB using the eigenvectors as the axes.
    XMVECTOR vMin, vMax;
    
    vMin = vMax = XMVector3TransformNormal( XMLoadFloat3(pPoints), InverseR );
    
    for (UINT i = 1; i < Count; i++)
    {
        XMVECTOR Point = XMVector3TransformNormal( XMLoadFloat3((XMFLOAT3*)((BYTE*)pPoints + i*Stride)), InverseR );
        
        vMin = XMVectorMin( vMin, Point );
        vMax = XMVectorMax( vMax, Point );
    }
    
    // Rotate the center into world space.
    XMVECTOR Center = (vMin + vMax) * 0.5f;
    Center = XMVector3TransformNormal( Center, R );

    // Store center, extents, and orientation.
    XMStoreFloat3( &pOut->Center, Center );
    XMStoreFloat3( &pOut->Extents, (vMax - vMin) * 0.5f );
    XMStoreFloat4( &pOut->Orientation, Orientation );

    return;
}

}; // namespace
