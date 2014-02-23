#version 120


attribute vec3 position;
attribute vec2 tex;
attribute vec3 normal;

uniform float camera_z;

varying vec2 tex_coord0;
varying vec3 normal0;

varying vec3 light_dir;
varying float cos_angle;


void main() {
  float r;
  //gl_TexCoord[0] = gl_MultiTexCoord0;
  //tex_coord0 = tex;
  //normal0    = (gl_ModelViewMatrix * vec4(normal, 0.0)).xyz;
  //world_pos0 = (gl_ModelViewMatrix * vec4(position, 1.0)).xyz;

  // Set the position of the current vertex
  gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
  gl_TexCoord[0].st = tex;
  tex_coord0 = tex;
  normal0 = normalize(gl_NormalMatrix * normal);
  //light_dir = normalize(vec3(-gl_LightSource[0].position));
  light_dir = normalize(vec3(gl_Position));
  //cos_angle = length(light_dir.xyz)*1000.0 / length(position.xyz);
  cos_angle = dot(normalize(position.xyz), normalize(light_dir.xyz));
/*  if (angle < 10.0)
    gl_FragColor = vec4(cf,af);
  else
    gl_FragColor = vec4(0.0, 0.0, 0.0, af);   */

  // position is the location on the model
  // 1000 * light_dir is the vector from the camera/light to the origin (the adjacent side)
  // hypotenuse is position - origin
  // cos(angle) = light_dir * 1000.0 / position;
}