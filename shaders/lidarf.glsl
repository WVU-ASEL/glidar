#version 120

/*
 * Copyright (c) 2014, John O. Woods, Ph.D.
 *   West Virginia University Applied Space Exploration Lab
 *   West Virginia Robotic Technology Center
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

varying vec3 normal0;

varying vec4 diffuse;
varying vec4 ambient;
varying vec4 specular;

varying vec3 half_vector;
varying vec3 ec_pos;

//uniform vec4 camera_pos;
uniform float far_plane;
uniform float near_plane;
uniform mat4 ViewMatrix;
uniform mat4 LightModelViewMatrix;


uniform sampler2D diffuse_texture_color;
uniform sampler2D specular_texture_color;

// Random number generator without any real testing done.
// Comes from: http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
//float rand(vec2 co) {
//  return fract(sin(dot(co.xy,vec2(12.9898,78.233))) * 43758.5453);
//}

float rand_positive(vec2 co) {
  return noise1(co) * 0.5 + 0.5;
}

float rand_0_mean(vec2 co) {
  return noise1(co*2);
}

void main() {
  const float GLOBAL_AMBIENT = 0.2;
  vec4 color = gl_FrontMaterial.emission; //vec4(gl_FrontMaterial.emission.r, 0.0, 0.0, 1.0);
  vec4 diffuse_color = diffuse * texture2D(diffuse_texture_color, gl_TexCoord[0].st); //vec4((diffuse * texture2D(diffuse_texture_color, gl_TexCoord[0].st)).r, 0.0, 0.0, 1.0);
  vec4 spec_color = specular * texture2D(specular_texture_color, gl_TexCoord[1].st); //vec4((specular * texture2D(specular_texture_color, gl_TexCoord[1].st)).r, 0.0, 0.0, 1.0);
  //float bounce = (specular * texture2D(specular_texture_color, gl_TexCoord[1].st)).r * rand_positive(gl_FragCoord.xy);

  vec3 spot_dir = vec3(ViewMatrix * vec4(gl_LightSource[0].spotDirection, 0.0));
  vec3 light_dir = -ec_pos; //spot_dir - ec_pos;

  float dist = length(light_dir);

  vec3 n = normalize(normal0);
  float n_dot_hv = max(dot(n, normalize(half_vector)), 0.0);

  color = GLOBAL_AMBIENT * ambient; //vec4(color.r + GLOBAL_AMBIENT * ambient.r, 0.0, 0.0, 1.0);

  if (n_dot_hv > 0.0) {

    // Calculate the angle w.r.t. the spotlight.
    float spot_effect = dot(normalize(spot_dir), normalize(light_dir));

    //if (spot_effect > gl_LightSource[0].spotCosCutoff) {
      float att = 1.0 / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);

      color += att * (diffuse_color * n_dot_hv + ambient);
      color += att * spec_color * pow(n_dot_hv, gl_FrontMaterial.shininess);

      //color.r *= rand_positive(gl_FragCoord.xy);

      float corrected_dist = spot_effect * dist;
      float dist_ratio = 65536.0f * (corrected_dist - near_plane) / (far_plane - near_plane);
      color.g = floor(dist_ratio / 256.0f) / 256.0;
      color.b = mod(dist_ratio, 256.0f) / 256.0;
      color.a = 1.0;
    //} else {
    //  color = vec4(0.0,0.0,0.0,1.0);
    //}
  } else {
    color = vec4(0.0,0.0,0.0,1.0);
  }

  gl_FragColor = color;
  //gl_FragDepth = dist;
}