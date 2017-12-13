#version 150
#extension GL_ARB_explicit_attrib_location : require

in vec2 pass_TexCoord;

out vec4 out_Color;

uniform sampler2D ColorTex;

 void main(){

 	out_Color = texture(ColorTex,pass_TexCoord);

 }