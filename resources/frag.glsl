#version 330
uniform vec3 UaColor;
uniform vec3 UdColor;
uniform vec3 UsColor;
uniform float Ushine;

in vec3 vCol;
in vec3 vNormal;
in vec3 vLight;
in vec3 vView;

out vec4 color;

void main() {

  vec3 Refl, Light, Norm, Spec, View, Diffuse;
  vec3 Half;

  Light = normalize(vLight);
  Norm = normalize(vNormal);
  View = normalize(vView);
  Half = Light+View;
  Spec = pow(clamp(dot(normalize(Half), Norm), 0.0, 1.0), Ushine)*UsColor;
  Diffuse = clamp(dot(Norm, Light), 0.0, 1.0)*UdColor;
  color = vec4((Diffuse + Spec + UaColor), 1);
}
