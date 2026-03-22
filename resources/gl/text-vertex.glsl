#version 130
in vec3 position;
in vec2 texcoord;
uniform mat4 mvp;
centroid out vec2 v_texcoord;

void main() {
  gl_Position = mvp * vec4(position, 1.0);
  v_texcoord = texcoord;
}
