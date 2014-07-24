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

varying vec4  diffuse;
varying vec4  ambient;
varying vec4  specular;

varying vec3 half_vector;
varying vec3 ec_pos;

uniform float camera_z;
uniform float far_plane;
uniform float near_plane;
uniform mat4 LightModelViewMatrix;

uniform sampler2D diffuse_texture_color;
uniform sampler2D specular_texture_color;

void main() {
  float GLOBAL_AMBIENT = 0.2;
  vec4 color = gl_FrontMaterial.emission;
  vec4 diffuse_color = diffuse * texture2D(diffuse_texture_color, gl_TexCoord[0].st);
  vec4 spec_color    = specular * texture2D(specular_texture_color, gl_TexCoord[1].st);

  vec3 light_dir = vec3(LightModelViewMatrix * gl_LightSource[0].position) - ec_pos;

  float dist = length(light_dir);

  vec3 n = normalize(normal0);

  float n_dot_l = max(dot(n, normalize(light_dir)), 0.0);

  //color += GLOBAL_AMBIENT * gl_FrontMaterial.ambient;


  if (n_dot_l > 0.0) {

    vec3 spot_dir = vec3(LightModelViewMatrix * vec4(gl_LightSource[0].spotDirection, 0.0));
    float spot_effect = dot(normalize(-spot_dir), normalize(-light_dir));

    if (spot_effect > gl_LightSource[0].spotCosCutoff) {
      float att = 1.0 / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);
      color += att * (diffuse_color * n_dot_l + ambient);

      vec3 half_v   = normalize(half_vector);
      float n_dot_hv = max(dot(n,half_v), 0.0);
      color += att * gl_FrontMaterial.specular * spec_color * pow(n_dot_hv, gl_FrontMaterial.shininess); // gl_FrontMaterial.specular * specular; //

      color.a = 1.0;
    } else {
      color.a = 1.0;
    }
  } else {
    color.a = 1.0;
  }

  gl_FragColor = color;
}