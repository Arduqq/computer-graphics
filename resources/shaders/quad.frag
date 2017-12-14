#version 150
#extension GL_ARB_explicit_attrib_location : require

in vec2 pass_TexCoord;

out vec4 out_Color;

uniform sampler2D ColorTex;
uniform int Mode;

 void main(){
	out_Color = texture(ColorTex,pass_TexCoord);
 	float avg = 0.2126 * out_Color.r + 0.7152 * out_Color.g + 0.0722 * out_Color.b;
 	out_Color = vec4(avg,avg,avg,1.0);
 }