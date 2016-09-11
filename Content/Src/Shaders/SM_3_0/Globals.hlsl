
// Params and declarations shared between all shaders in a render path.
// They are set once at the start of the frame, so all shaders must be
// compatible to these declarations.

matrix ViewProj: register(c0) <string CBuffer = "CameraParams"; int SlotIndex = 0;>;
float3 EyePos: register(c4) <string CBuffer = "CameraParams"; int SlotIndex = 0;>;
