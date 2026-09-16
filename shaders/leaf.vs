#version 330

// Instanced falling leaves: a unit plane stood upright, tumbled and spun
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 instA;   // position xyz, size
in vec4 instB;   // spin (rad), tumble (rad), alpha, colour index

uniform mat4 mvp;

out vec2 fragTexCoord;
out vec3 fragWorldPos;
out vec4 fragColor;

const vec3 LEAF_COLORS[3] = vec3[3](vec3(0.71, 0.18, 0.12), vec3(0.82, 0.47, 0.16), vec3(0.78, 0.67, 0.2));

void main() {
    // Plane lies in XZ; stand it up (rotate about X), tumble about X, spin about Y
    vec3 p = vertexPosition * instA.w;
    p = vec3(p.x, -p.z, p.y);
    float ct = cos(instB.y), st = sin(instB.y);
    p = vec3(p.x, ct * p.y - st * p.z, st * p.y + ct * p.z);
    float cy = cos(instB.x), sy = sin(instB.x);
    p = vec3(cy * p.x + sy * p.z, p.y, -sy * p.x + cy * p.z);
    vec3 world = instA.xyz + p;
    fragTexCoord = vertexTexCoord;
    fragWorldPos = world;
    fragColor = vec4(LEAF_COLORS[int(instB.w)], instB.z);
    gl_Position = mvp * vec4(world, 1.0);
}
