#ifndef COMMON_H
#define COMMON_H

//  Definitions and functions needed by all shaders.
//  (C) 2007 Radon Labs GmbH

#define CompileTargetVS vs_3_0
#define CompileTargetPS ps_3_0
//------------------------------------------------------------------------------
/**
    Techniques: Cullmode and AlphaRef must be declared in calling shader,
                its value isn't set by every shadernodeinstance 
                (depends on nebula2 shadersettings in maya), so don't forget 
                to set it to a correct default values  

*///------------------------------------------------------------------------------
#define DepthTechnique(Name, features, vertexShader, pixelShader) \
technique Name < string Mask = features; > \
{ \
    pass p0 \
    { \
        VertexShader    = compile CompileTargetVS vertexShader(); \
        PixelShader     = compile CompileTargetPS pixelShader();\
        CullMode        = <CullMode>; \
    } \
}

#define SimpleTechnique(Name, features, vertexShader, pixelShader, UseATest) \
technique Name < string Mask = features; > \
{ \
    pass p0 \
    { \
        VertexShader    = compile CompileTargetVS vertexShader(); \
        PixelShader     = compile CompileTargetPS pixelShader(UseATest);\
        CullMode        = <CullMode>; \
    } \
}

#define LitTechnique(Name, features, vertexShader, pixelShader, UseATest, LightCount, UseParallax) \
technique Name < string Mask = features; > \
{ \
    pass p0 \
    { \
        VertexShader    = compile CompileTargetVS vertexShader(LightCount); \
        PixelShader     = compile CompileTargetPS pixelShader(UseATest, LightCount, UseParallax);\
        CullMode        = <CullMode>; \
    } \
}

#define SimpleTechniqueNoCulling(name, features, vertexShader, pixelShader) \
technique name < string Mask = features; > \
{ \
    pass p0 \
    { \
        VertexShader    = compile CompileTargetVS vertexShader(); \
        PixelShader     = compile CompileTargetPS pixelShader(); \
        CullMode        = None; \
    } \
}

#define AlphaTechnique(name, features, vertexShader, pixelShader, useLight) \
technique name < string Mask = features; > \
{ \
    pass p0 \
    { \
        VertexShader    = compile CompileTargetVS vertexShader(); \
        PixelShader     = compile CompileTargetPS pixelShader(useLight);\
        CullMode        = <CullMode>; \
    } \
}

#endif

