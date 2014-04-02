#ifndef __LIGHTING_H__
#define __LIGHTING_H__

// Lighting library (different models)

//!!!can add subsurface scattering!

// Naming conventions for this file:
// N - surface normal vector
// L - surface point to the ligth direction
// V - surface point to the viewer direction
// R - reflection vector that is vector opposite to light (L) against surface normal (N)
//     normalize(2.0f * dot(N, L) * N - L) or normalize(-reflect(L, N))
// H - half-vector between viewer (V) and light (L) vectors
//     normalize(L + V)
// CT - Cook-Torrance model
// ON - Oren-Nayar model

// Final color in CT example:
// max(NdotL, 0.f) * (cSpecular * CT() + cDiffuse);
// CT() = (D * F * G) / (4.f * NdotV * NdotL)

// Final color in ON example:
// ON() * cDiffuse;
// ON() = (A + B * max(Gamma, 0.f) * C) * max(NdotL, 0.f)

#define LIGHT_DIR	0
#define LIGHT_POINT	1
#define LIGHT_SPOT	2

#define Attenuation			AttenuationQuad
#define SpotlightFalloff	SpotlightFalloffLinear

// Light params
#define MAX_LIGHTS 4
shared float4	LightAmbient;				//???shared for all scene? not per-light?
shared int		LightType[MAX_LIGHTS];
shared float3	LightPos[MAX_LIGHTS];
shared float3	LightDir[MAX_LIGHTS];		// NB: light direction must be pre-inverted for directional lights
shared float4	LightColor[MAX_LIGHTS];		// Pre-multiplied by intensity
shared float3	LightParams[MAX_LIGHTS];	// x - range, y - cos half theta, z - cos half phi //!!!can pre-invert range!

// Specific material params
float SpecularExponent = 75.f;	// Phong
float Roughness = 0.25f;		// CT, ON: radians, [0, PI/2] //!!!used only as Roughness * Roughness in both CT and ON microfacet models!
float RefAtNormalInc = 0.8f;	// CT: reflectance at normal incidence (i.e., the value of the Fresnel term when angle = 0 or minimal reflection)

// Diffuse intensity functions ****************************************

float DiffuseLambert(float3 N, float3 L)
{
	return max(dot(N, L), 0.0f);
}   
//---------------------------------------------------------------------

//???pre-square Roughness on CPU? ALWAYS used as square!
float DiffuseOrenNayar(float3 N, float3 L, float3 V)
{
// See another impl in: http://content.gpwiki.org/index.php/D3DBook:(Lighting)_Oren-Nayar

	float NdotL = dot(N, L); //???max(dot(), 0.0f);
	float NdotV = dot(N, V); //???max(dot(), 0.0f);

	float SqRgh = Roughness * Roughness;
	float A = 1.0f - 0.5f * (SqRgh / (SqRgh + 0.33f)); // In D3DBook: (SqRgh + 0.57f));
	float B = 0.45f * (SqRgh / (SqRgh + 0.09f));

	float Gamma = dot(V - N * NdotV, L - N * NdotL);

	// Can use lookup texture:
	// UV was: float2((NdotV + 1.0f) / 2.0f, (NdotL + 1.0f) / 2.0f)
	// float C = tex2D(LookupMap, float2(NdotV, NdotL) * 0.5f + 0.5f).x;

	//???does float2(NdotV, NdotL) cost something? mb initially store in float2?
	float2 Angles = acos(float2(NdotV, NdotL));
	float C = sin(max(Angles.x, Angles.y)) * tan(min(Angles.x, Angles.y));

	return (A + B * max(Gamma, 0.f) * C) * max(NdotL, 0.f);
}
//---------------------------------------------------------------------

// Specular intensity functions ***************************************

float SpecularPhong(float3 N, float3 L, float3 V)
{
	//???negate reflect() or resulting dot()? is there any PERF difference?
	return pow(max(dot(normalize(-reflect(L, N)), V), 0.0f), SpecularExponent);
}
//---------------------------------------------------------------------

float SpecularBlinnPhong(float3 N, float3 L, float3 V)
{
	return pow(max(dot(normalize(L + V), N), 0.0f), SpecularExponent);
}
//---------------------------------------------------------------------

float SpecularBlinnPhongVSHalfVector(float3 N, float3 H)
{
	return pow(max(dot(H, N), 0.0f), SpecularExponent);
}
//---------------------------------------------------------------------

// Roughness in example was 10.0f
float SpecularWardIsotropic(float3 N, float3 L, float3 V)
{
	float NdotH = max(dot(N, normalize(L + V)), 0.f);
	float NdotH_2 = NdotH * NdotH;
	return exp(-Roughness * (1.f - NdotH_2) / NdotH_2);
}
//---------------------------------------------------------------------

//???pre-square Roughness on CPU? ALWAYS used as square!
float SpecularCookTorrance(float3 N, float3 L, float3 V)
{
// See another impl in: http://content.gpwiki.org/index.php/D3DBook:(Lighting)_Cook-Torrance
// They saturate all dots. It isn't necessary if all vectors are normalized.

	float3	H = normalize(L + V);
	float	NdotL = dot(N, L);				//???max(dot(N, L), 0.0f);
	float	NdotV = dot(N, V);				//???max(dot(N, V), 0.0f);
	float	VdotH = dot(V, H);				//???max(dot(V, H), 0.0f);
	float	NdotH = max(dot(N, H), 1.0e-7f); //???max(..., 0.f)? why not 0.f?
	float	NdotH_2 = NdotH * NdotH;

	// Can use lookup texture:
	// float D = tex2D(LookupMap, float2(Roughness * Roughness, NdotH)).x;

	// Beckmann distribution factor
	float Inv_NdotH_2_Rgh_2 = 1.0f / (NdotH_2 * Roughness * Roughness);
	float D = exp((NdotH_2 - 1.0f) * Inv_NdotH_2_Rgh_2) * Inv_NdotH_2_Rgh_2 / NdotH_2;

	// Fresnel term (Schlick's approximation)
	//!!!TRY! Original: F = (1.f + NdotV) ^ Lambda; (F = 1.f / (1.f + NdotV); for Lambda = -1)
	//???mb difficulti is because of dynamic Lambda value?
	float F = RefAtNormalInc + (1.0f - RefAtNormalInc) * pow(1.0f - VdotH, 5.0f);

	// Geometric attenuation term
	float G = min(1.0f, 2.0f * NdotH / VdotH * min(NdotV, NdotL));

	return (D * F * G) / (4.f * NdotV * NdotL);
	//return max((D * F * G) / (4.f * NdotV * NdotL), 0.f);
}
//---------------------------------------------------------------------

// Attenuation functions **********************************************

float AttenuationLinear(float Distance, float Range)
{
	return saturate(1 - Distance / Range);
}
//---------------------------------------------------------------------

// The most realistic
float AttenuationQuad(float Distance, float Range)
{
	float Factor = Distance / Range;
	return saturate(1 - Factor * Factor);
	// return 1 / (Distance * Distance); - not normalized
}
//---------------------------------------------------------------------

float AttenuationExp(float Distance, float Range)
{
	return pow(saturate(1 - Distance / Range), 0.5f); //???where to get exp? per-light?
}
//---------------------------------------------------------------------

/*
float Attenuation
    ( 
        float distance, 
        float range, 
        float a, 
        float b, 
        float c
    )
{
    float Atten = 1.0f / ( a * distance * distance + b * distance + c );
 
    // Use the step() intrinsic to clamp light to 
    // zero out of its defined range
    return step(distance, range) * saturate( atten );
}
*/

// Falloff functions **************************************************

// CosAlpha = dot(LightDir, SurfacePos - LightPos)

// Simple case with a falloff exponent = 1
float SpotlightFalloffLinear(float CosAlpha, float CosHalfTheta, float CosHalfPhi)
{
//!!!can precompute InvDenominator on CPU!
	return saturate((CosAlpha - CosHalfPhi) / (CosHalfTheta - CosHalfPhi));
}
//---------------------------------------------------------------------

float SpotlightFalloffHermite(float CosAlpha, float CosHalfTheta, float CosHalfPhi)
{
	// -2 * x^3 + 3 * x^2 (from shader asm code)
	return smoothstep(CosHalfPhi, CosHalfTheta, CosAlpha);
}
//---------------------------------------------------------------------

float SpotlightFalloffExp(float CosAlpha, float CosHalfTheta, float CosHalfPhi, float FalloffExp)
{
	return pow(SpotlightFalloffLinear(CosAlpha, CosHalfTheta, CosHalfPhi), FalloffExp);
}
//---------------------------------------------------------------------

#endif

/*
VS_TS_OUTPUT vsTangentSpace( in VS_INPUT v )
{
    VS_TS_OUTPUT o = (VS_TS_OUTPUT)0;
 
    // Transform the incoming model-space position to
    // projection space - a standard VS operation.
    o.position = mul( float4( v.position, 1.0f ), mWorldViewProj );
 
    // The world-space vertex position is useful for
    // later calculations.
    float3 pWorld = mul( float4( v.position, 1.0f ), mWorld ).xyz;
 
    // The per-vertex attributes are in model space, so we
    // need to transform them to world space before we can
    // generate a world->tangent transformation
    float3 vTangentWS   = mul( v.tangent,   (float3x3)mWorld );
    float3 vBitangentWS = mul( v.bitangent, (float3x3)mWorld );
    float3 vNormalWS    = mul( v.normal,    (float3x3)mWorld );
 
    // Pack the data into the matrix for transforming
    // the actual input data
    float3x3 mWorldToTangentSpace;
    mWorldToTangentSpace[0] = normalize( vTangentWS );
    mWorldToTangentSpace[1] = normalize( vBitangentWS );
    mWorldToTangentSpace[2] = normalize( vNormalWS );
 
    // Use the matrix to transform all necessary
    // inputs for the pixel shader.
    o.lightdir = mul( mWorldToTangentSpace, -vLightDir );
 
    o.viewdir = pCameraPosition - pWorld;
    o.viewdir = mul( mWorldToTangentSpace, o.viewdir );
 
    // Pass through any other necessary attributes
    o.texcoord = v.texcoord;
 
    return o;
}

float4 psMain( in VS_TS_OUTPUT_V ) : SV_TARGET
{
    // Normalize the input vectors
    float3 view  = normalize( v.viewdir );
    float3 light = normalize( v.lightdir );
 
    // Determine the offset for this pixel
    float2 offset = ComputeOffset( v.texcoord, view );
 
    // Retrieve the normal
    float3 normal = FetchNormalVector( v.texcoord, true );
 
    // Evaluate the lighting model
    float4 colour = ComputeIllumination( normal, view, light );
 
    // Return the final colour
    return colour;
}
*/

/*
float4 psStrauss( in VS_OUTPUT f ) : SV_TARGET
{
    // Make sure the interpolated inputs and
    // constant parameters are normalized
    float3 n = normalize( f.normal );
    float3 l = normalize( -vLightDirection );
    float3 v = normalize( pCameraPosition - f.world );
    float3 h = reflect( l, n );
 
    // Declare any aliases:
    float NdotL   = dot( n, l );
    float NdotV   = dot( n, v );
    float HdotV   = dot( h, v );
    float fNdotL  = fresnel( NdotL );
    float s_cubed = fSmoothness * fSmoothness * fSmoothness;
 
    // Evaluate the diffuse term
    float d  = ( 1.0f - fMetalness * fSmoothness );
    float Rd = ( 1.0f - s_cubed ) * ( 1.0f - fTransparency );
    float3 diffuse = NdotL * d * Rd * cDiffuse;
 
    // Compute the inputs into the specular term
    float r = ( 1.0f - fTransparency ) - Rd;
 
    float j = fNdotL * shadow( NdotL ) * shadow( NdotV );
 
    // 'k' is used to provide small off-specular
    // peak for very rough surfaces. Can be changed
    // to suit desired results...
    const float k = 0.1f;
    float reflect = min( 1.0f, r + j * ( r + k ) );
 
    float3 C1 = float3( 1.0f, 1.0f, 1.0f );
    float3 Cs = C1 + fMetalness * (1.0f - fNdotL) * (cDiffuse - C1);
 
    // Evaluate the specular term
    float3 specular = Cs * reflect;
    specular *= pow( -HdotV, 3.0f / (1.0f - fSmoothness) );
 
    // Composite the final result, ensuring
    // the values are >= 0.0f yields better results. Some
    // combinations of inputs generate negative values which
    // looks wrong when rendered...
    diffuse  = max( 0.0f, diffuse );
    specular = max( 0.0f, specular );
    return float4( diffuse + specular, 1.0f );
}

float4 psWardIsotropic
        ( 
            in VS_OUTPUT f, 
            uniform bool UseLookUpTexture 
        ) : SV_TARGET
{
    // Make sure the interpolated inputs and
    // constant parameters are normalized
    float3 n = normalize( f.normal );
    float3 l = normalize( -vLightDirection );
    float3 v = normalize( pCameraPosition - f.world );
    float3 h = normalize( l + v );
 
    // Generate any useful aliases
    float VdotN = dot( v, n );
    float LdotN = dot( l, n );
    float HdotN = dot( h, n );
    float r_sq = (fRoughness * fRoughness) + 1e-5f;
    // (Adding a small bias to r_sq stops unexpected
    //  results caused by divide-by-zero)
 
    // Define material properties
    float3 Ps = float3( 1.0f, 1.0f, 1.0f );
 
    // Compute the specular term
    float exp_a;
    if( UseLookUpTexture )
    {
        // Map the -1.0..+1.0 dot products to
        // a 0.0..1.0 range suitable for a
        // texture look-up.
        float tc = float2
                    ( 
                        (HdotN + 1.0f) / 2.0f, 
                        0.0f 
                    );
        exp_a = texIsotropicLookup.Sample( DefaultSampler, tc ).r;
    }
    else
    {
        // manually compute the complex term
        exp_a = -pow( tan( acos( HdotN ) ), 2 );
    }
    float spec_num = exp( exp_a / r_sq );
 
    float spec_den = 4.0f * 3.14159f * r_sq;
    spec_den *= sqrt( LdotN * VdotN );
 
    float3 Specular = Ps * ( spec_num / spec_den );
 
    // Composite the final value:
    return float4( dot( n, l ) * (cDiffuse + Specular ), 1.0f );
}

float4 psWardAnisotropic
        (
            in VS_OUTPUT f
        ) : SV_TARGET
{
    // Make sure the interpolated inputs and
    // constant parameters are normalized
    float3 n = normalize( f.normal );
    float3 l = normalize( -vLightDirection );
    float3 v = normalize( pCameraPosition - f.world );
    float3 h = normalize( l + v );
 
    // Apply a small bias to the roughness
    // coefficients to avoid divide-by-zero
    fAnisotropicRoughness += float2( 1e-5f, 1e-5f );
 
    // Define the coordinate frame
    float3 epsilon   = float3( 1.0f, 0.0f, 0.0f );
    float3 tangent   = normalize( cross( n, epsilon ) );
    float3 bitangent = normalize( cross( n, tangent ) );
 
    // Define material properties
    float3 Ps   = float3( 1.0f, 1.0f, 1.0f );
 
    // Generate any useful aliases
    float VdotN = dot( v, n );
    float LdotN = dot( l, n );
    float HdotN = dot( h, n );
    float HdotT = dot( h, tangent );
    float HdotB = dot( h, bitangent );
 
    // Evaluate the specular exponent
    float beta_a  = HdotT / fAnisotropicRoughness.x;
    beta_a       *= beta_a;
 
    float beta_b  = HdotB / fAnisotropicRoughness.y;
    beta_b       *= beta_b;
 
    float beta = -2.0f * ( ( beta_a + beta_b ) / ( 1.0f + HdotN ) );
 
    // Evaluate the specular denominator
    float s_den  = 4.0f * 3.14159f; 
    s_den       *= fAnisotropicRoughness.x;
    s_den       *= fAnisotropicRoughness.y;
    s_den       *= sqrt( LdotN * VdotN );
 
    // Compute the final specular term
    float3 Specular = Ps * ( exp( beta ) / s_den );
 
    // Composite the final value:
    return float4( dot( n, l ) * (cDiffuse + Specular ), 1.0f );
}

        ( 
            in VS_OUTPUT f
        ) : SV_TARGET
{
    // Make sure the interpolated inputs and
    // constant parameters are normalized
    float3 n = normalize( f.normal );
    float3 l = normalize( -vLightDirection );
    float3 v = normalize( pCameraPosition - f.world );
    float3 h = normalize( l + v );
 
    // Define the coordinate frame
    float3 epsilon = float3( 1.0f, 0.0f, 0.0f );
    float3 tangent = normalize( cross( n, epsilon ) );
    float3 bitangent = normalize( cross( n, tangent ) );
 
    // Generate any useful aliases
    float VdotN = dot( v, n );
    float LdotN = dot( l, n );
    float HdotN = dot( h, n );
    float HdotL = dot( h, l );
    float HdotT = dot( h, tangent );
    float HdotB = dot( h, bitangent );
 
    float3 Rd = cDiffuse;
    float3 Rs = 0.3f;
 
    float Nu = fAnisotropy.x;
    float Nv = fAnisotropy.y;
 
    // Compute the diffuse term
    float3 Pd = (28.0f * Rd) / ( 23.0f * 3.14159f );
    Pd *= (1.0f - Rs);
    Pd *= (1.0f - pow(1.0f - (LdotN / 2.0f), 5.0f));
    Pd *= (1.0f - pow(1.0f - (VdotN / 2.0f), 5.0f));
 
    // Compute the specular term
    float ps_num_exp = Nu * HdotT * HdotT + Nv * HdotB * HdotB;
    ps_num_exp /= (1.0f - HdotN * HdotN);
 
    float Ps_num = sqrt( (Nu + 1) * (Nv + 1) );
    Ps_num *= pow( HdotN, ps_num_exp );
 
    float Ps_den = 8.0f * 3.14159f * HdotL;
    Ps_den *= max( LdotN, VdotN );
 
    float3 Ps = Rs * (Ps_num / Ps_den);
    Ps *= ( Rs + (1.0f - Rs) * pow( 1.0f - HdotL, 5.0f ) );
 
    // Composite the final value:
    return float4( Pd + Ps, 1.0f );
}
*/