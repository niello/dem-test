#ifndef __NORMAL_MAPPING_H__
#define __NORMAL_MAPPING_H__

// Normal mapping library (bump, parallax)

float NMHeightScale = 0.05f;

texture NormalMap;

sampler NMSampler = sampler_state
{
	Texture   = <NormalMap>;
	AddressU  = Wrap;
	AddressV  = Wrap;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	MipMapLodBias = -0.5; //???need?
};

float3 FetchNormal(float2 UV)
{
	return normalize(tex2D(NMSampler, UV).rgb - .5f);
}
//---------------------------------------------------------------------

// Height is in range [0 .. 1], so Height - 0.5 results in [-0.5 .. 0.5] variations 
float2 ParallaxUV(float2 UV, float3 ViewDir)
{
	return (((tex2D(NMSampler, UV).a - .5f) * NMHeightScale) * ViewDir.xy) + UV;
}
//---------------------------------------------------------------------

/*
float3 ParallaxOcclusionMapping
        ( 
            float2 pStart, 
            float3 vDir, 
            uniform float HEIGHT_SCALE, 
            uniform uint MIN_SAMPLES, 
            uniform uint MAX_SAMPLES, 
            uniform bool COUNT_SAMPLES_TAKEN 
        )
{
    // Compute initial parallax displacement direction:
    float2 vParallaxDirection = normalize( vDir.xy );
 
    // The length of this vector determines the
    // furthest amount of displacement:
    float fLength          = length( vDir );
    fLength               *= fLength;
    float vDirZ_sq         = vDir.z * vDir.z;
    float fParallaxLength  = sqrt( fLength - vDirZ_sq ) 
    fParallaxLength       /= vDir.z; 
 
    // Compute the actual reverse parallax displacement vector:
    float2 vParallaxOffset = vParallaxDirection * fParallaxLength;
 
    // Need to scale the amount of displacement to account
    // for different height ranges in height maps.
    vParallaxOffset *= HEIGHT_SCALE;
 
    // corrected for tangent space. Normal is always z=1 in TS and 
    // v.viewdir is in tangent space as well...
    uint nNumSteps = (int)lerp
                            ( 
                                MAX_SAMPLES, 
                                MIN_SAMPLES, 
                                normalize( vDir ).z 
                            );
 
    float fCurrHeight        = 0.0;
    float fStepSize          = 1.0 / (float) nNumSteps;
    uint nStepIndex          = 0;
    float2 vTexCurrentOffset = pStart;
    float2 vTexOffsetPerStep = fStepSize * vParallaxOffset;
 
    float3 rVal = float3
                    ( 
                        vTexCurrentOffset 
                        - ( vTexOffsetPerStep * nNumSteps ),
                        0.0f
                    );
 
    float dx = ddx( pStart.x );
    float dy = ddy( pStart.y );
 
    float fPrevHeight = 1.0;
    float curr_ray_dist = 1.0f;
 
    while ( nStepIndex < nNumSteps ) 
    {
        // Determine where along our ray we currently are.
        curr_ray_dist -= fStepSize;
 
        vTexCurrentOffset -= vTexOffsetPerStep;
 
        fCurrHeight = texHeightMap.SampleGrad
                                    ( 
                                        DefaultSampler, 
                                        vTexCurrentOffset, 
                                        dx, 
                                        dy 
                                    ).r;
 
        if( COUNT_SAMPLES_TAKEN ) 
            rVal.z += ( 1.0f / (MAX_SAMPLES - MIN_SAMPLES) );
 
        // Because we're using heights in the [0..1] range 
        // and the ray is defined in terms of [0..1] scanning
        // from top-bottom we can simply compare the surface 
        // height against the current ray distance.
        if ( fCurrHeight >= curr_ray_dist ) 
        {
            // Push the counter above the threshold so that
            // we exit the loop on the next iteration
            nStepIndex = nNumSteps + 1;
 
            // We now know the location along the ray of the first
            // point *BELOW* the surface and the previous point 
            // *ABOVE* the surface:
            float ray_dist_above = curr_ray_dist 
                                   + ( 1.0f / (float)nNumSteps );
            float ray_dist_below = curr_ray_dist;
 
            // We also know the height of the surface before and
            // after we intersected it:
            float surf_height_before = fPrevHeight;
            float surf_height_after = fCurrHeight;
 
            float numerator = ray_dist_above - surf_height_before;
            float denominator = 
                  (surf_height_after - surf_height_before) 
                  - (ray_dist_below - ray_dist_above);
 
            // As the angle between the view direction and the
            // surface becomes closer to parallel (e.g. grazing
            // view angles) the denominator will tend towards zero.
            // When computing the final ray length we'll 
            // get a divide-by-zero and bad things happen.
 
            float x = 0.0f;
 
            if( all( denominator ) )
                x = numerator / denominator;
 
            // Now that we've found the position along the ray
            // that indicates where the true intersection exists
            // we can translate this into a texture coordinate 
            // - the intended output of this utility function.
 
            rVal.xy = lerp
                        ( 
                            vTexCurrentOffset + vTexOffsetPerStep, 
                            vTexCurrentOffset, 
                            x 
                        );
        }
        else
        {
            ++nStepIndex;
            fPrevHeight = fCurrHeight;
        }
    }
 
    return rVal;
}

// ========= refined and with self-shadowing ==================

float2 vParallaxDirection = normalize(  v.lightdir.xy );
 
float fLength          = length( vDir );
fLength               *= fLength;
float vDirZ_sq         = vDir.z * vDir.z;
float fParallaxLength  = sqrt( fLength - vDirZ_sq ) 
fParallaxLength       /= vDir.z; 
 
float2 vParallaxOffset = vParallaxDirection * fParallaxLength;
 
vParallaxOffset *= HEIGHT_SCALE;
 
uint nNumSteps = (int)lerp
                        ( 
                            MAX_SAMPLES, 
                            MIN_SAMPLES, 
                            normalize( v.lightdir ).z 
                        );
 
float  fCurrHeight       = 0.0;
float  fStepSize         = 1.0 / (float) nNumSteps;
uint   nStepIndex        = 0;
float2 vTexCurrentOffset = uv.xy;
float2 vTexOffsetPerStep = fStepSize * vParallaxOffset;
float  fCurrentBound     = 1.0;
 
float dx                 = ddx( v.texcoord.x );
float dy                 = ddy( v.texcoord.y );
 
float fPrevHeight        = 1.0;
 
float initial_height = texHeightMap.Sample
                                    (
                                        DefaultSampler,
                                        uv.xy
                                    ).r + SHADOW_OFFSET;
 
if( SHOW_SAMPLE_COUNT )
    uv.z += ( 1.0f / ( (MAX_SAMPLES - MIN_SAMPLES) + 1 ) );
 
while( nStepIndex < nNumSteps )
{
    vTexCurrentOffset += vTexOffsetPerStep;
 
    float RayHeight = lerp
                         (
                            initial_height,
                            1.0f,
                            nStepIndex / nNumSteps
                          );
 
    fCurrHeight = texHeightMap.SampleGrad
                                 (
                                     DefaultSampler,
                                     vTexCurrentOffset,
                                     dx,
                                     dy
                                 ).r;
    if( SHOW_SAMPLE_COUNT )
        uv.z += ( 1.0f / ( (MAX_SAMPLES - MIN_SAMPLES) + 1 ) );
 
    if( fCurrHeight > RayHeight )
    {
        // ray has gone below the height of the surface, therefore
        // this pixel is occluded...
        s = 1.0f;
        nStepIndex = nNumSteps;
    }
 
    ++nStepIndex;
}

*/

#endif
