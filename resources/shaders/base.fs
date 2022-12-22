#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

uniform vec3 u_viewPos;
uniform vec3 u_fogDepth;
uniform vec4 u_fogColor;
uniform vec3 u_lightPosition[4];
uniform vec4 u_lightColor[4];
uniform vec2 u_lightSteps[4];
uniform vec4 u_lightAmbient;

int bayer[8 * 8] = int[](
 0,32, 8,40, 2,34,10,42,
48,16,56,24,50,18,58,26,
12,44, 4,36,14,46, 6,38,
60,28,52,20,62,30,54,22,
 3,35,11,43, 1,33, 9,41,
51,19,59,27,49,17,57,25,
15,47, 7,39,13,45, 5,37,
63,31,55,23,61,29,53,21
);
float bayerSize = 8.0;
float bayerDivider = bayerSize * bayerSize;

vec4 nearestColour(vec4 inColor) {
    vec4 color = inColor;
    color.r = floor((32 - 1.0) * color.r + 0.5) / (32 - 1.0);
    color.g = floor((32 - 1.0) * color.g + 0.5) / (32 - 1.0);
    color.b = floor((32 - 1.0) * color.b + 0.5) / (32 - 1.0);
	color.a = floor((8 - 1.0) * color.a + 0.5) / (8 - 1.0);
    return color;
}

void main() {
	float spread = 1.0 / (0.299 * (32.0 - 1.0) + 0.587 * (32.0 - 1.0) + 0.114 * (32.0 - 1.0));
    vec4 color = fragColor * texture(texture0, fragTexCoord);
	vec2 entry = mod(gl_FragCoord.xy, vec2(bayerSize, bayerSize));

	if(u_lightAmbient.w != 0) {
		vec3 lightCol = vec3(0.0);
        for (int i = 0; i < 4; i++) {
            if(u_lightColor[i].w == 1) {
				vec3 normal = normalize(fragNormal);
				vec3 lightDir = normalize(u_lightPosition[i] - fragPosition);
				float diffuse = dot(normal, lightDir);
				float diffuseToon = max(ceil(diffuse * u_lightSteps[i].x) / u_lightSteps[i].x, 0.0);
				lightCol += (diffuseToon * u_lightColor[i].xyz);
            }
        }
		color = vec4(color.xyz * (u_lightAmbient.xyz+lightCol), color.w);
	}
	if(u_fogDepth.z != 0) {
		float depth = length(u_viewPos - fragPosition);
		float fogDensity = clamp((depth-u_fogDepth.x)/(u_fogDepth.y-u_fogDepth.x), 0.0, 1.0);
        finalColor = nearestColour(vec4(mix(color.xyz, u_fogColor.xyz, fogDensity), color.w));
	} else {
		finalColor = nearestColour(color);
	}
}
