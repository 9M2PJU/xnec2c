#version 130
in vec3 position;
in vec3 normal;
in vec4 color;
uniform mat4 mvp;
out vec4 vertexColor;
out vec3 viewNormal;
out vec3 viewPos;
out vec3 worldPos;

void main() {
  vec4 pos = mvp * vec4(position, 1.0);
  gl_Position = pos;
  viewPos = pos.xyz / pos.w;
  viewNormal = normalize(mat3(mvp) * normal);
  vertexColor = color;
  worldPos = position;
}
