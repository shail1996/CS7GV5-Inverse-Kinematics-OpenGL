#version 410

in vec3 vertex_position;
in vec3 vertex_normals;

out vec3 n_eye;
out vec3 position_eye;
out vec3 normal_eye;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;
uniform float specularChangeVal;

void main(){
	n_eye = (view * vec4 (vertex_normals, 0.0)).xyz;	
	position_eye = vec3 (view * model * vec4 (vertex_position, 1.0));
	normal_eye = vec3 (view * model * vec4 (vertex_normals, 0.0));
	gl_Position =  proj * view * model * vec4 (vertex_position, 1.0);
}