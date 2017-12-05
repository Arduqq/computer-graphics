#version 150
#extension GL_ARB_explicit_attrib_location : require
// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;
layout(location = 3) in vec3 in_Tangent;

//Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform vec3 NormalTex;

out vec3 pass_Normal;
out vec3 lightDirection;
out vec3 cameraDirection;
out vec2 pass_TexCoord;
out vec3 pass_Tangent;

void main(void)
{
	
	vec4 sunPosition = ViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	vec4 worldPosition = (ViewMatrix * ModelMatrix) * vec4(in_Position, 1.0);

    pass_Normal = normalize((NormalMatrix * vec4(in_Normal, 0.0)).xyz);
    lightDirection = normalize(sunPosition.xyz - worldPosition.xyz);
    cameraDirection = normalize(-1*(worldPosition.xyz));
    pass_TexCoord = in_TexCoord;
    pass_Tangent = in_Tangent;


    gl_Position = ProjectionMatrix * worldPosition;
}