#pragma once
#include <iostream>
#include <string>
using namespace std;

const string instancing_vert = R"(
#version 330

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;
uniform float scale;

layout (location = 0) in vec3 position;
// attibute at location 1 will be set to have
// "attribute divisor = 1" which means for given buffer for attibute 1,
// index for reading that buffer will increase per-instance, not per-vertex
// divisor = 0 is default value and means per-vertex increase
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec3 normal;
layout (location = 4) in vec3 offset;
layout (location = 5) in mat4 rotate;
layout (location = 9) in vec4 myColor;

out vec4 outColor;
out vec3 FragPos;
out vec3 Normal;

void main()
{
  mat4 scale = mat4(scale, 0.0, 0.0, 0.0, 
                    0.0, scale, 0.0, 0.0,  
                    0.0, 0.0, scale, 0.0,  
                    0.0, 0.0, 0.0, 1.0);
  mat4 translation = mat4(1.0, 0.0, 0.0, 0.0, 
                          0.0, 1.0, 0.0, 0.0,  
                          0.0, 0.0, 1.0, 0.0,  
                          offset.x, offset.y, offset.z, 1.0);                 
  vec4 p = vec4(position, 1.0);
  
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * 
                translation * rotate * scale * p;
  outColor = myColor;
  Normal = normal;
  FragPos = vec3(al_ModelViewMatrix * translation * rotate * scale * p);
}
)";

const string instancing_frag = R"(
#version 330
out vec4 frag_out0;

in vec4 outColor;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos; 
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
  vec3 ambient = vec3(1.0, 1.0, 1.0) * lightColor;

  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(lightPos - FragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * lightColor * vec3(1.0, 1.0, 1.0);

  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
  vec3 specular = lightColor * (spec * vec3(0.5, 0.5, 0.5));

  frag_out0 = vec4(vec3(outColor) * (ambient + diffuse + specular), 1.0);
}
)";