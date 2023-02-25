//-------------------------------------------------------------------------------------
//  ParseMaterial.h
//
//  Entry points for FBX material parsing.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

using namespace ATG;

ExportMaterial* ParseMaterialInLayer( KFbxMesh* pMesh, KFbxLayer* pLayer, DWORD dwMaterialIndex );
