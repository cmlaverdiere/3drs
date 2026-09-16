#version 330

// 3x3 tent upsample, accumulated additively into the next larger level
in vec2 fragTexCoord;
uniform sampler2D uSource;
uniform vec2 uTexel;
out vec4 finalColor;

void main() {
    vec2 o = uTexel;
    vec3 sum = texture(uSource, fragTexCoord).rgb * 4.0;
    sum += (texture(uSource, fragTexCoord + vec2(-o.x, 0)).rgb + texture(uSource, fragTexCoord + vec2(o.x, 0)).rgb +
            texture(uSource, fragTexCoord + vec2(0, -o.y)).rgb + texture(uSource, fragTexCoord + vec2(0, o.y)).rgb) * 2.0;
    sum += texture(uSource, fragTexCoord + vec2(-o.x, -o.y)).rgb + texture(uSource, fragTexCoord + vec2(o.x, -o.y)).rgb +
           texture(uSource, fragTexCoord + vec2(-o.x, o.y)).rgb + texture(uSource, fragTexCoord + vec2(o.x, o.y)).rgb;
    finalColor = vec4(sum / 16.0, 1.0);
}
