# DynamicShaderLinkageFX11

This is the DirectX SDK's Direct3D 11 sample updated to use the Windows 10 SDK without any dependencies on legacy DirectX SDK content. This sample is a Win32 desktop DirectX 11.0 application for Windows 10, Windows 8.1, Windows 8, and Windows 7. 

## Description

This Direct3D 11 sample demonstrates use of Shader Model 5 shader interfaces and Direct3D 11 support for linking shader interface methods at runtime.

*This is an Effects 11 version of the Direct3D 11 sample DynamicShaderLinkage11.  This requires Feature Level 10.0 or better due to the shader complexity and feature use.*

## Coping With The Shader Permutation Explosion

Rendering systems need to deal with a wide range of complexity when managing shaders while providing the greatest possible opportunity to optimize shader code. This becomes an even greater challenge when you factor in the need to support a variety of different materials in a rendered scene across the broad range of available hardware configurations. To address these challenges, shader developers have often resorted to one of two general approaches. Either creating fully featured "uber-shaders" which trade off some performance for flexibility or specifically creating individual shaders for each geometry stream, material type or light type combination needed.

Uber-shaders handle this combinatorial problem by recompiling the same shader with different preprocessor defines, while the latter method uses brute-force developer power to the same end. This shader permutation explosion has often been a problem for developers who must now manage thousands of different shader permutations within their game and asset pipeline.

Direct3D 11 and shader model 5 introduces Object Oriented language constructs and provides runtime support of shader linking to assist developers in tackling these development problems. This sample uses shader interfaces to help manage the number of shader permutations created.

## Shader Model 5 Interfaces and Classes

In the pixel shader, we first define base interfaces for different light and material types:

```
   //--------------------------------------------------------------------------------------
   // Interfaces
   //--------------------------------------------------------------------------------------
   interface iBaseLight
   {
   float3 IlluminateAmbient(float3 vNormal);
   float3 IlluminateDiffuse(float3 vNormal);
   float3 IlluminateSpecular(float3 vNormal, int specularPower );
   };
   
   // ...
   interface iBaseMaterial
   {
   float3 GetAmbientColor(float2 vTexcoord);
   float3 GetDiffuseColor(float2 vTexcoord);
   int GetSpecularPower();
   };
   // ...
```

Next we build specialized classes based on these interfaces, adding to the functionality where needed:

```cpp
//--------------------------------------------------------------------------------------
// Classes
//--------------------------------------------------------------------------------------
class cAmbientLight : iBaseLight
{
   float3 m_vLightColor;
   bool     m_bEnable;
   float3 IlluminateAmbient(float3 vNormal);
   float3 IlluminateDiffuse(float3 vNormal);
   float3 IlluminateSpecular(float3 vNormal, int specularPower );

   };
   // ...

   class cBaseMaterial : iBaseMaterial
   {

   float3 m_vColor;

   int      m_iSpecPower;
   float3 GetAmbientColor(float2 vTexcoord);
   float3 GetDiffuseColor(float2 vTexcoord);
  
   int GetSpecularPower();
   };

   // ....
```

## Abstract Instances and The Shaders Main Function

The pixel shaders main function uses abstract instances of the interfaces for computation. These interfaces instances are made concrete by the application code at shader bind time:

```
//--------------------------------------------------------------------------------------
// Abstract Interface Instances for dyamic linkage / permutation
//--------------------------------------------------------------------------------------
iBaseLight     g_abstractAmbientLighting;
iBaseLight     g_abstractDirectLighting;
iBaseMaterial  g_abstractMaterial;
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain( PS_INPUT Input ) : SV_TARGET
{
    // Compute the Ambient term
    float3   Ambient = (float3)0.0f;
    Ambient = g_abstractMaterial.GetAmbientColor( Input.vTexcoord ) * g_abstractAmbientLighting.IlluminateAmbient( Input.vNormal );
    // Accumulate the Diffuse contribution
    float3   Diffuse = (float3)0.0f;
    Diffuse += g_abstractMaterial.GetDiffuseColor( Input.vTexcoord ) * g_abstractDirectLighting.IlluminateDiffuse( Input.vNormal );
    // Compute the Specular contribution
    float3   Specular = (float3)0.0f;
    Specular += g_abstractDirectLighting.IlluminateSpecular( Input.vNormal, g_abstractMaterial.GetSpecularPower() );
    // Accumulate the lighting with saturation
    float3 Lighting = saturate( Ambient + Diffuse + Specular );
return float4(Lighting,1.0f);

}
```

Here we are simply accumulating the lighting for the pixel across the generic / abstract light instances.

## Application Code and Shader Interfaces

Once the shader has been compiled and loaded, the sample application code makes use of the shader reflection layer to acquire both the concrete and abstract shader class instance offsets within the compiled shader. These offsets are then used to dynamically configure the shader code path according to user selected options. The linkage happens directly in the Direct3D 11 runtime and is specified during the PSSetShader call

### Acquisition of the Shader Interfaces

After shader compilation we must acquire the offsets for the abstract instances of our permutable shader objects:

```cpp
// use shader reflection to get data locations for the interface array
ID3D11ShaderReflection* pReflector = NULL;
V_RETURN( D3DReflect( pPixelShaderBuffer->GetBufferPointer(), pPixelShaderBuffer->GetBufferSize(),
IID_ID3D11ShaderReflection, (void**) &pReflector) );
g_iNumPSInterfaces = pReflector->GetNumInterfaceSlots();
g_dynamicLinkageArray = (ID3D11ClassInstance**) malloc( sizeof(ID3D11ClassInstance*) * g_iNumPSInterfaces );
if (g_dynamicLinkageArray == NULL)
return E_FAIL;
ID3D11ShaderReflectionVariable* pAmbientLightingVar = pReflector->GetVariableByName("g_abstractAmbientLighting");
g_iAmbientLightingOffset = pAmbientLightingVar->GetInterfaceSlot(0);
ID3D11ShaderReflectionVariable* pDirectLightingVar = pReflector->GetVariableByName("g_abstractDirectLighting");
g_iDirectLightingOffset = pDirectLightingVar->GetInterfaceSlot(0);
ID3D11ShaderReflectionVariable* pMaterialVar = pReflector->GetVariableByName("g_abstractMaterial");
g_iMaterialOffset = pMaterialVar->GetInterfaceSlot(0);
           
// ...
```

Next we enumerate all possible permutations of material object that exist in the shader:

```cpp
// Material Dynamic Permutation
enum E_MATERIAL_TYPES
{
   MATERIAL_PLASTIC,
   MATERIAL_PLASTIC_TEXTURED,
   MATERIAL_PLASTIC_LIGHTING_ONLY,
   MATERIAL_ROUGH,
   MATERIAL_ROUGH_TEXTURED,
   MATERIAL_ROUGH_LIGHTING_ONLY,
   MATERIAL_TYPE_COUNT
};
char*  g_pMaterialClassNames[ MATERIAL_TYPE_COUNT ] =
{
   "g_plasticMaterial",             // cPlasticMaterial
   "g_plasticTexturedMaterial",     // cPlasticTexturedMaterial
   "g_plasticLightingOnlyMaterial", // cPlasticLightingOnlyMaterial
   "g_roughMaterial",               // cRoughMaterial
   "g_roughTexturedMaterial",       // cRoughTexturedMaterial
   "g_roughLightingOnlyMaterial"    // cRoughLightingOnlyMaterial
};
E_MATERIAL_TYPES            g_iMaterial = MATERIAL_PLASTIC_TEXTURED;

// ...
// Acquire the material Class Instances for all possible material settings
for( UINT i=0; i < MATERIAL_TYPE_COUNT; i++)
{
   g_pPSClassLinkage->GetClassInstance( g_pMaterialClassNames[i], 0, &g_pMaterialClasses[i] );
}
```

### Specifying Linkage to the Runtime

Users specify the specific lighting and material configuration through the samples user interface. In the application code, at Shader bind time the following selectively sets shader code bindings according to user specified settings:

```cpp
// Setup the Shader Linkage based on the user settings for Lighting
// Ambient Lighting First - Constant or Hemi?
if ( g_bHemiAmbientLighting )
g_dynamicLinkageArray[g_iAmbientLightingOffset] = g_pHemiAmbientLightClass;
else
g_dynamicLinkageArray[g_iAmbientLightingOffset] = g_pAmbientLightClass;
// Direct Light - None or Directional TODO: or Spot or Environment?
if (g_bDirectLighting)
g_dynamicLinkageArray[g_iDirectLightingOffset] = g_pDirectionalLightClass;
else
g_dynamicLinkageArray[g_iDirectLightingOffset] = g_pAmbientLightClass;

// Use the selected material class instance
if (g_bLightingOnly)
{
switch( g_iMaterial )
{
case MATERIAL_PLASTIC:
case MATERIAL_PLASTIC_TEXTURED:
g_dynamicLinkageArray[g_iMaterialOffset] = g_pMaterialClasses[ MATERIAL_PLASTIC_LIGHTING_ONLY ];
break;
case MATERIAL_ROUGH:
case MATERIAL_ROUGH_TEXTURED:
g_dynamicLinkageArray[g_iMaterialOffset] = g_pMaterialClasses[ MATERIAL_ROUGH_LIGHTING_ONLY ];
break;
}
}
else
g_dynamicLinkageArray[g_iMaterialOffset] =     g_pMaterialClasses[ g_iMaterial ] ;
```

The Direct3D 11 runtime efficiently links each of the selected methods at source level, inlining and optimizing the shader code as much as possible to provide an optimal shader for the GPU to execute. 

## Dependencies

DXUT-based samples typically make use of runtime HLSL compilation. Build-time compilation is recommended for all production Direct3D applications, but for experimentation and samples development runtime HLSL compilation is preferred. Therefore, the D3DCompile*.DLL must be available in the search path when these programs are executed.

> When using the Windows 10 SDK and targeting Windows 7 or later, you can include the D3DCompile_47 DLL side-by-side with your application copying the file from the REDIST folder. ``%ProgramFiles(x86)%\Windows kits\10\Redist\D3D\x86 or x64``

## More information

[DXUT for Win32 Desktop Update](https://walbourn.github.io/dxut-for-win32-desktop-update/)   
[Effects for Direct3D 11 Update](https://walbourn.github.io/effects-for-direct3d-11-update/)
