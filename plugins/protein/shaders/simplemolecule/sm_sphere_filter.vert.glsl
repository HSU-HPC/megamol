#version 110

#include "simplemolecule/sm_common_defines.glsl"
#include "simplemolecule/sm_common_input.glsl"

attribute float filter;

varying float squarRad;
varying float rad;

void main(void) {
    // remove the sphere radius from the w coordinates to the rad varyings
    vec4 inPos = gl_Vertex;
    rad = inPos.w;
    squarRad = rad * rad;
    inPos.w = 1.0;
    
    // object pivot point in object space    
    objPos = inPos; // no w-div needed, because w is 1.0 (Because I know)

    // calculate cam position
    camPos = MVinv[3]; // (C) by Christoph
    camPos.xyz -= objPos.xyz; // cam pos to glyph space

    // calculate light position in glyph space
    // USE THIS LINE TO GET POSITIONAL LIGHTING
    //lightPos = MVinv * gl_LightSource[0].position - objPos;
    // USE THIS LINE TO GET DIRECTIONAL LIGHTING
    lightPos = MVinv * normalize( gl_LightSource[0].position);

    // send color to fragment shader
    gl_FrontColor = gl_Color; 

    // Sphere-Touch-Plane-Approach
    vec2 winHalf = 2.0 / viewAttr.zw; // window size

    vec2 d, p, q, h, dd;

    // get camera orthonormal coordinate system
    vec4 tmp;

    // camera coordinate system in object space
    tmp = MVinv[3] + MVinv[2];
    vec3 camIn = normalize(tmp.xyz);
    tmp = MVinv[3] + MVinv[1];
    vec3 camUp = tmp.xyz;
    vec3 camRight = normalize(cross(camIn, camUp));
    camUp = cross(camIn, camRight);

    vec2 mins, maxs;
    vec3 testPos;
    vec4 projPos;
    
    // projected camera vector
    vec3 c2 = vec3(dot(camPos.xyz, camRight), dot(camPos.xyz, camUp), dot(camPos.xyz, camIn));

    vec3 cpj1 = camIn * c2.z + camRight * c2.x;
    vec3 cpm1 = camIn * c2.x - camRight * c2.z;

    vec3 cpj2 = camIn * c2.z + camUp * c2.y;
    vec3 cpm2 = camIn * c2.y - camUp * c2.z;
    
    d.x = length(cpj1);
    d.y = length(cpj2);

    dd = vec2(1.0) / d;

    p = squarRad * dd;
    q = d - p;
    h = sqrt(p * q);
    //h = vec2(0.0);
    
    p *= dd;
    h *= dd;

    cpj1 *= p.x;
    cpm1 *= h.x;
    cpj2 *= p.y;
    cpm2 *= h.y;

    // TODO: rewrite only using four projections, additions in homogenous coordinates and delayed perspective divisions.
    testPos = objPos.xyz + cpj1 + cpm1;
    projPos = MVP * vec4(testPos, 1.0);
    projPos /= projPos.w;
    mins = projPos.xy;
    maxs = projPos.xy;

    testPos -= 2.0 * cpm1;
    projPos = MVP * vec4(testPos, 1.0);
    projPos /= projPos.w;
    mins = min(mins, projPos.xy);
    maxs = max(maxs, projPos.xy);

    testPos = objPos.xyz + cpj2 + cpm2;
    projPos = MVP * vec4(testPos, 1.0);
    projPos /= projPos.w;
    mins = min(mins, projPos.xy);
    maxs = max(maxs, projPos.xy);

    testPos -= 2.0 * cpm2;
    projPos = MVP * vec4(testPos, 1.0);
    projPos /= projPos.w;
    mins = min(mins, projPos.xy);
    maxs = max(maxs, projPos.xy);

    gl_Position = vec4((mins + maxs) * 0.5, 0.0, 1.0);
    gl_PointSize = max((maxs.x - mins.x) * winHalf.x, (maxs.y - mins.y) * winHalf.y) * 0.5;

#ifdef SMALL_SPRITE_LIGHTING
    // for normal crowbaring on very small sprites
    lightPos.w = (clamp(gl_PointSize, 1.0, 5.0) - 1.0) / 4.0;
#endif // SMALL_SPRITE_LIGHTING

    // Apply filter if necessary
    if(filter == 0.0) {
        gl_PointSize = 0.0;
    }
}