#version 120

// ganked with modifications from: http://inst.eecs.berkeley.edu/~cs184/fa12/online/glshading.pdf

uniform mat4 mv;

uniform sampler2D texture_color;

attribute vec3 camera_pos;

varying vec3 normal;
attribute vec4 vertex;

uniform int is_texture;
uniform int is_light;

uniform vec3 light_dir;
uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;
uniform float shininess;

vec4 compute_light(const in vec3 direction, const in vec3 normal_, const in vec3 half_vector, const in vec4 diffuse_,
                   const in vec4 specular_, const in float shininess_) {
  float n_dot_l = dot(normal, direction);
  vec4 lambert  = diffuse_ * vec4(1,1,1,1) * max(n_dot_l, 0.0);

  float n_dot_h = dot(normal, half_vector);
  vec4 phong    = specular_ * vec4(1,1,1,1) * pow(max(n_dot_h, 0.0), shininess_);

  vec4 return_value = lambert + phong;

  return return_value;
}

void main (void) {
  if (is_texture > 0) gl_FragColor = texture2D(texture_color, gl_TexCoord[0].st);
  else if (is_light == 0) gl_FragColor = vec4(1,1,1,1); // white
  else {
    vec4 vertex_pos4 = mv * vertex;
    vec3 vertex_pos3 = vertex_pos4.xyz / vertex_pos4.w; // dehomogenize
    vec3 camera_dir  = normalize(camera_pos - vertex_pos3);

    // Compute normal, needed for shading.
    vec3 normal3 = (gl_ModelViewMatrixInverseTranspose * vec4(normal, 0.0)).xyz;
    vec3 normal_normalized = normalize(normal3);

    // Light 0
    vec3 direction0 = normalize(light_dir);
    vec3 half0 = normalize(direction0 + camera_dir);
    vec4 color0 = compute_light(direction0, normal_normalized, half0, diffuse, specular, shininess);

    gl_FragColor = ambient + color0;
  }

}