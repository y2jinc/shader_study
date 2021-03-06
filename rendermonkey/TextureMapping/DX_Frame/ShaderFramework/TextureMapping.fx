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
// TextureMapping
//--------------------------------------------------------------//
//--------------------------------------------------------------//
// Pass 0
//--------------------------------------------------------------//
string Effect_Group_1_TextureMapping_Pass_0_Model : ModelData = ".\\sphere.x";

float4x4 gWorldMatrix : World;
float4x4 gViewMatrix : View;
float4x4 gProjMatrix : Projection;

struct VS_INPUT 
{
   float4 Position : POSITION;
   float2 mTexCoord : TEXCOORD0;   
};

struct VS_OUTPUT 
{
   float4 Position : POSITION;
   float2 mTexCoord : TEXCOORD0;   
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
   VS_OUTPUT Output;

   Output.Position = mul( Input.Position, gWorldMatrix );
   Output.Position = mul( Output.Position, gViewMatrix );
   Output.Position = mul( Output.Position, gProjMatrix );
   
   Output.mTexCoord = Input.mTexCoord;
   
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
};

float4 ps_main(PS_INPUT Input) : COLOR0
{
   float4 albedo = tex2D(DiffuseSampler, Input.mTexCoord);
   return albedo.rgba;
}

//--------------------------------------------------------------//
// Technique Section for Effect Workspace.Effect Group 1.TextureMapping
//--------------------------------------------------------------//
technique TextureMapping
{
   pass Pass_0
   {
      VertexShader = compile vs_2_0 vs_main();
      PixelShader = compile ps_2_0 ps_main();
   }

}