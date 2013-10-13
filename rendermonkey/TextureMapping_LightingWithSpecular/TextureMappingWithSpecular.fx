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
// TextureMapping_LightingWithSpecular
//--------------------------------------------------------------//
//--------------------------------------------------------------//
// Pass 0
//--------------------------------------------------------------//
string Effect_Group_1_TextureMapping_LightingWithSpecular_Pass_0_Model : ModelData = ".\\sphere.x";

float4x4 gWorldMatrix : World;
float4x4 gViewMatrix : View;
float4x4 gProjMatrix : Projection;

float4 gLightPosition
<
   string UIName = "gLightPosition";
   string UIWidget = "Direction";
   bool UIVisible =  false;
   float4 UIMin = float4( -10.00, -10.00, -10.00, -10.00 );
   float4 UIMax = float4( 10.00, 10.00, 10.00, 10.00 );
   bool Normalize =  false;
> = float4( 100.00, 200.00, 0.00, 1.00 );
float4 gCameraPosition : ViewPosition;

struct VS_INPUT 
{
   float4 Position : POSITION;
   float3 Normal : NORMAL;
   float2 mTexCoord : TEXCOORD0;   
};

struct VS_OUTPUT 
{
   float4 Position : POSITION;
   float2 mTexCoord : TEXCOORD0;   
   float3 mDiffuse : TEXCOORD1;
   float3 mViewDir : TEXCOORD2;
   float3 mReflection : TEXCOORD3;
};

VS_OUTPUT Effect_Group_1_TextureMapping_LightingWithSpecular_Pass_0_Vertex_Shader_vs_main( VS_INPUT Input )
{
   VS_OUTPUT Output;
   float3 WorldNormal;
   float3 LightDir;
   float3 ViewDir;

   Output.Position = mul( Input.Position, gWorldMatrix );
   WorldNormal = mul( Input.Normal, gWorldMatrix );
   WorldNormal = normalize(WorldNormal);
   
   LightDir = (float3)Output.Position.xyz - gLightPosition.xyz;
   LightDir = normalize(LightDir);
   
   ViewDir = (float3)Output.Position - gCameraPosition.xyz;
   ViewDir = normalize(ViewDir);
   
   Output.Position = mul( Output.Position, gViewMatrix );
   Output.Position = mul( Output.Position, gProjMatrix );
   
   Output.mTexCoord = Input.mTexCoord;
   Output.mDiffuse = dot(-LightDir, WorldNormal);
   Output.mViewDir = ViewDir;   
   Output.mReflection = reflect(LightDir, WorldNormal);
   
   return( Output );
   
}




texture DiffuseMap_Tex
<
   string ResourceName = "C:\\Program Files (x86)\\AMD\\RenderMonkey 1.82\\Examples\\Media\\Textures\\Earth.jpg";
>;
sampler2D DiffuseSampler = sampler_state
{
   Texture = (DiffuseMap_Tex);
};
struct PS_INPUT
{
   float2 mTexCoord : TEXCOORD0;
   float3 mDiffuse : TEXCOORD1;
   float3 mViewDir : TEXCOORD2;
   float3 mReflection : TEXCOORD3;
};

float4 Effect_Group_1_TextureMapping_LightingWithSpecular_Pass_0_Pixel_Shader_ps_main(PS_INPUT Input) : COLOR0
{
   float3 diffuse = tex2D(DiffuseSampler, Input.mTexCoord);
   float3 diffuseLit = saturate(Input.mDiffuse);
   
   float3 reflection = normalize(Input.mReflection);
   float3 ViewDir = normalize(Input.mViewDir);
   float3 specular = 0;
   if ( diffuse.x > 0 )
   {
      specular = saturate(dot(reflection, -ViewDir));
      specular = pow(specular, 20);
   }
   
   float3 ambient = float3(0.1f, 0.1f, 0.1f);
   return float4(ambient+specular+(diffuse*diffuseLit),1);
   //return float4(ambient+specular+(diffuseLit),1);
//   return float4(specular+(,1);
}
//--------------------------------------------------------------//
// Technique Section for Effect Group 1
//--------------------------------------------------------------//
technique TextureMapping_LightingWithSpecular
{
   pass Pass_0
   {
      VertexShader = compile vs_2_0 Effect_Group_1_TextureMapping_LightingWithSpecular_Pass_0_Vertex_Shader_vs_main();
      PixelShader = compile ps_2_0 Effect_Group_1_TextureMapping_LightingWithSpecular_Pass_0_Pixel_Shader_ps_main();
   }

}

