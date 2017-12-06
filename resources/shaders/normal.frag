#version 150

in vec3 pass_Normal;
in vec4 vertexPosition;
in vec2 pass_TexCoord;
in vec3 pass_Tangent;

out vec4 out_Color;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform sampler2D ColorTex;
uniform sampler2D NormalTex;

const vec3 specularColor = vec3(0.6, 0.6, 0.6);
const vec3 ambientColor = vec3(0.5, 0.5, 0.5);
const vec3 diffuseColor = vec3(0.3, 0.3, 0.3); 
const float glance = 16.0;

uniform int mode;

void main() {
  vec3 planetColor = texture(ColorTex, pass_TexCoord).rgb;
  vec3 planetNormal = (vec3((texture(NormalTex, pass_TexCoord).xy * 2.0 - 1.0), texture(NormalTex, pass_TexCoord).z));
  vec4 lightPosition = ViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
  vec4 worldPosition = (ViewMatrix * ModelMatrix) * vertexPosition;

  vec3 bitangents = normalize(cross(pass_Normal, pass_Tangent));
  mat3 tangents = mat3(pass_Tangent, bitangents, pass_Normal);

  planetNormal = tangents * planetNormal;
  vec3 normal = normalize(planetNormal);
  vec3 light = normalize(lightPosition.xyz - worldPosition.xyz);
  vec3 vertex = normalize(-worldPosition.xyz);

  float lambertian = max(dot(light, normal), 0.0);

  float specular = 0.0;
  if(lambertian > 0.0) {
    vec3 halfDir = normalize(light + vertex); // halfway vector
    float specularAngle = max(dot(halfDir, normal), 0.0);
    specular = pow(specularAngle, glance);
  }

  out_Color = vec4(planetColor + lambertian * diffuseColor + specular * specularColor, 1.0);
}
