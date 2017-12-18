#version 150
#extension GL_ARB_explicit_attrib_location : require

in vec2 pass_TexCoord;

out vec4 out_Color;

uniform sampler2D ColorTex;
uniform bool Greyscale;
uniform bool Gaussian;

const float blurSizeH = 1.0 / 3000.0;
const float blurSizeV = 1.0 / 3000.0;

void main(){
	out_Color = texture(ColorTex,pass_TexCoord);
	if (Greyscale && !Gaussian) {
		float avg = 0.2126 * out_Color.r + 0.7152 * out_Color.g + 0.0722 * out_Color.b;
 		out_Color = vec4(avg,avg,avg,1.0);
	}

	else if (!Greyscale && Gaussian) {
    	vec4 sum = vec4(0.0);
    	for (int x = -4; x <= 4; x++)
    	    for (int y = -4; y <= 4; y++)
    	        sum += texture(ColorTex, vec2(pass_TexCoord.x + x * blurSizeH, pass_TexCoord.y + y * blurSizeV)
    	        ) / 81.0;
   	 	out_Color = sum;
	}

	else if (Greyscale && Gaussian) {
 		vec4 sum = vec4(0.0);
    	for (int x = -4; x <= 4; x++)
    	    for (int y = -4; y <= 4; y++)
    	        sum += texture(ColorTex, vec2(pass_TexCoord.x + x * blurSizeH, pass_TexCoord.y + y * blurSizeV)
    	        ) / 81.0;
   	 	out_Color = sum;
   	 	
		float avg = 0.2126 * out_Color.r + 0.7152 * out_Color.g + 0.0722 * out_Color.b;
 		out_Color = vec4(avg,avg,avg,1.0);
	}

	else if (!Greyscale && !Gaussian) {
		out_Color = texture(ColorTex,pass_TexCoord);
	}
 
}