#version 150
#extension GL_ARB_explicit_attrib_location : require
// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;

//Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform vec3 ColorVector;
uniform sampler2D ColorTex;

out vec3 pass_Normal;
out vec3 lightDirection;
out vec3 cameraDirection;
out vec3 planetColor;

void main(void)
{
	
	vec4 sunPosition = ViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	vec4 worldPosition = (ViewMatrix * ModelMatrix) * vec4(in_Position, 1.0);

    pass_Normal = normalize((NormalMatrix * vec4(in_Normal, 0.0)).xyz);
    lightDirection = normalize(sunPosition.xyz - worldPosition.xyz);
    cameraDirection = normalize(-1*(worldPosition.xyz));

    planetColor = texture(ColorTex, in_TexCoord).xyz;

    gl_Position = ProjectionMatrix * worldPosition;
}