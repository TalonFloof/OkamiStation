#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

// NOTE: Add here your custom variables

void main() {
    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = vec3(matModel*vec4(vertexNormal, 0.0));

    // Calculate final vertex position
    vec2 resolution = vec2(256,192);
	vec4 vert = mvp*vec4(vertexPosition, 1.0);
	vec3 norm = (matModel*vec4(vertexNormal, 0.0)).xyz;

	// Snap vertices to screen pixels
	vec4 snapped = vert;
	snapped.xyz = vert.xyz / vert.w;
	snapped.xy = floor(resolution * snapped.xy) / resolution;
	snapped.xyz *= vert.w;

    gl_Position = snapped;

    fragColor = vertexColor;
}
