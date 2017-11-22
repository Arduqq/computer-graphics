#version 150
#extension GL_ARB_explicit_attrib_location : require

uniform vec3 ColorVector;

const float numShades = 6;
const float offset = 0.4;

const vec3 specularColor = vec3(0.2, 0.2, 0.2);
const vec3 ambientColor = vec3(0.5, 0.5, 0.5);
const vec3 diffuseColor = vec3(0.3, 0.3, 0.3); 

in vec3 pass_Normal;
in vec3 lightDirection;
in vec3 cameraDirection;
in vec3 planetColor;

out vec4 out_Color;


float diffuseSimple(vec3 L, vec3 N){
   return clamp(dot(L,N),0.0,1.0);
}


float specularSimple(vec3 L,vec3 N,vec3 H){
   if(dot(N,L)>0){
      return pow(clamp(dot(H,N),0.0,1.0),64.0);
   }
   return 0.0;
}

void main() {
   vec3 light = normalize(lightDirection);
   vec3 vertex = normalize(cameraDirection);
   vec3 normal = normalize(pass_Normal);

   float dotView = dot(pass_Normal, cameraDirection);
	if(dotView < offset){
		out_Color = vec4(planetColor, 1.0);
	} else {
		float amb = 0.1;
		float dif = diffuseSimple(normal, light);
		float spe = specularSimple(normal, light, vertex);
		float intensity = amb + dif + spe;
		float shadeIntensity = ceil(intensity * numShades)/numShades;
		out_Color = vec4((planetColor * shadeIntensity), 1.0);
	}
}
