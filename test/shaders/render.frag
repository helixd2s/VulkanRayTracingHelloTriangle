#version 460 core
#extension GL_EXT_ray_query : enable
#extension GL_EXT_ray_tracing : enable

layout (location = 0) in vec2 vcoord;
layout (location = 0) out vec4 fragColor;

layout (binding = 0, set = 0) uniform AccelerationStructure { uvec2 acc; };
//layout (binding = 0, set = 0) uniform accelerationStructureEXT acc;
layout (binding = 1, rgba32f) uniform image2D img;

void main(){
    //fragColor = vec4(imageLoad(img, ivec2(gl_FragCoord.xy)).xyz, 1.f);//vec4(1.f, 0.5f, 0.f, 1.f);

    vec2 size = vec2(1280.f.x, 720.f.x);
    //vec2 pixelCenter = vec2(gl_FragCoord.xy) + vec2(0.5);
    //vec2 uv = pixelCenter / size;

    vec2 d = vcoord * 2.0 - 1.0;
    float aspectRatio = size.x / size.y;

    vec3 rayOrigin = vec3(0, 0, -1.5);
    vec3 rayDir = normalize(vec3(d.x * aspectRatio, d.y, 1));

    vec4 payload = vec4(0.3f, 0.3f, 0.3f, 1.f);

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, accelerationStructureEXT(acc), gl_RayFlagsTerminateOnFirstHitEXT|gl_RayFlagsOpaqueEXT, 0xff, rayOrigin, 0.001, rayDir, 100.0);

    while(rayQueryProceedEXT(rayQuery)) {
        rayQueryConfirmIntersectionEXT(rayQuery);
    };

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
        vec2 attribs = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
        payload = vec4(1.f - attribs.x - attribs.y, attribs.x, attribs.y, 1.f);
    };

    fragColor = payload;
    //imageStore(img, ivec2(gl_FragCoord.xy), payload);   
};
