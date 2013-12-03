//**************************************************************//
//  Effect File exported by RenderMonkey 1.6
//
//  - Although many improvements were made to RenderMonkey FX  
//    file export, there are still situations that may cause   
//    compilation problems once the file is exported, such as  
//    occasional naming conflicts for methods, since FX format 
//    does not support any notions of name spaces. You need to 
//    try to create workspaces in such a way as to minimize    
//    potential naming conflicts on export.                    
//    
//  - Note that to minimize resulting name collisions in the FX 
//    file, RenderMonkey will mangle names for passes, shaders  
//    and function names as necessary to reduce name conflicts. 
//**************************************************************//

//--------------------------------------------------------------//
// Effect Group 1
//--------------------------------------------------------------//
//--------------------------------------------------------------//
// ShadowMapping
//--------------------------------------------------------------//
//--------------------------------------------------------------//
// ApplyShadowTorus
//--------------------------------------------------------------//
string Effect_Group_1_ShadowMapping_ApplyShadowTorus_Torus : ModelData = ".\\Torus.x";

struct VS_INPUT
{
   float4 mPosition : POSITION;
   float4 mNormal : NORMAL;
};

struct VS_OUTPUT
{
   float4 mPosition : POSITION;
   float4 mClipPosition : TEXCOORD1;
   float mDiffuse : TEXCOORD2;
};

float4x4 gWorldMatrix : World;
float4x4 gLightViewMatrix
<
   string UIName = "gLightViewMatrix";
   string UIWidget = "Numeric";
   bool UIVisible =  false;
> = float4x4( 1.00, 0.00, 0.00, 0.00, 0.00, 1.00, 0.00, 0.00, 0.00, 0.00, 1.00, 0.00, 0.00, 0.00, 0.00, 1.00 );
float4x4 gLightProjectionMatrix : Projection;

float4 gWorldLightPosition
<
   string UIName = "gWorldLightPosition";
   string UIWidget = "Direction";
   bool UIVisible =  false;
   float4 UIMin = float4( -10.00, -10.00, -10.00, -10.00 );
   float4 UIMax = float4( 10.00, 10.00, 10.00, 10.00 );
   bool Normalize =  false;
> = float4( 100.00, 500.00, -500.00, 1.00 );

float4x4 gViewProjectionMatrix : ViewProjection;

VS_OUTPUT vs_main( VS_INPUT Input )
{
   VS_OUTPUT Output;
   
   float4x4 lightViewMatrix = gLightViewMatrix;
   
   float4 worldPosition = mul(Input.mPosition, gWorldMatrix);
   Output.mPosition = mul(worldPosition, gViewProjectionMatrix);
   
   Output.mClipPosition = mul(worldPosition, lightViewMatrix);
   Output.mClipPosition = mul(Output.mClipPosition, gLightProjectionMatrix);
   
   float3 lightDir = normalize(worldPosition.xyz - gWorldLightPosition.xyz);
   float3 worldNormal = normalize(mul(Input.mNormal, (float3x3)gWorldMatrix));
   Output.mDiffuse = dot(-lightDir, worldNormal);
   
   return Output;
}
texture ShadowMap_Tex
<
   string ResourceName = "..\\..\\";
>;
sampler2D ShadowSampler = sampler_state
{
   Texture = (ShadowMap_Tex);
};
float4 gObjectColor
<
   string UIName = "gObjectColor";
   string UIWidget = "Direction";
   bool UIVisible =  false;
   float4 UIMin = float4( -10.00, -10.00, -10.00, -10.00 );
   float4 UIMax = float4( 10.00, 10.00, 10.00, 10.00 );
   bool Normalize =  false;
> = float4( 1.00, 1.00, 0.00, 1.00 );

struct PS_INPUT
{
   float4 mClipPosition : TEXCOORD1;
   float4 mDiffuse : TEXCOORD2;
};

float4 ps_main(PS_INPUT Input) : COLOR
{
   float3 diffuse = float3(saturate(Input.mDiffuse).xxx);
   float3 rgb = diffuse * gObjectColor;
   
   float currentDepth = Input.mClipPosition.z / Input.mClipPosition.w;
   
   float2 uv = Input.mClipPosition.xy / Input.mClipPosition.w;
   uv.y = -uv.y;
   uv = uv * 0.5 + 0.5;
   
   float shadowDepth = tex2D(ShadowSampler, uv).r;
   
   if (currentDepth > shadowDepth + 0.0000125f)
   {
      rgb *= 0.5f;
   }
   
   return ( float4(rgb.xyz, 1) );
}
//--------------------------------------------------------------//
// Technique Section for Effect Group 1
//--------------------------------------------------------------//
technique ShadowMapping
{
   pass ApplyShadowTorus
   {
      VertexShader = compile vs_2_0 vs_main();
      PixelShader = compile ps_2_0 ps_main();
   }
}

