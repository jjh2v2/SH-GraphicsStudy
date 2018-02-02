#version 330 core

out vec3 Normal;
out vec3 FragmentPos;
out vec2 TexCoord;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0f);

	Normal = mat3(transpose(inverse(view * model))) * aNormal;
	FragmentPos = vec3(view * model * vec4(aPos, 1.0));

	TexCoord = aTexCoord;
}