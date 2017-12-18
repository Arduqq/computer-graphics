#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;

out vec2 pass_TexCoord;

uniform bool MirrorH;
uniform bool MirrorV;

void main(){
	vec2 coord = in_TexCoord;
	if (MirrorH && MirrorV) {
		coord.x = -coord.x;
		coord.y = -coord.y;
	}
	else if (!MirrorH && MirrorV) {
		coord.x = -coord.x;
	}
	else if (MirrorH && !MirrorV) {
		coord.y = -coord.y;
	}
	else {
		gl_Position = vec4(in_Position,1.0);
	}
	pass_TexCoord = coord;
}