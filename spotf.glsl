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

varying vec3 light_dir;

varying vec4  diffuse;
varying vec4  ambient;
varying vec4  specular;

varying vec3 half_vector;
varying vec3 ec_pos;

uniform float camera_z;

uniform sampler2D texture_color;

void main() {
  const float GLOBAL_AMBIENT = 0.2;
  float n_dot_l, n_dot_hv, att;
  vec3 cf;
  vec4 color = gl_FrontMaterial.emission * texture2D(texture_color, gl_TexCoord[0].st);
  float spot_effect;
  float SHININESS = 128.0;
  float CATT = 0.9, LATT = 0.01, QATT = 0.01, SPOT_EXP = 0.01;

  vec3 light_dir0 = normalize(vec3(0.0, 0.0, camera_z) - ec_pos);

  float dist = length(light_dir0);

  n_dot_l = max(dot(normal0, normalize(light_dir0)), 0.0);

  color += GLOBAL_AMBIENT * gl_FrontMaterial.ambient;


  if (n_dot_l > 0.0) {

    spot_effect = dot(normalize(light_dir), normalize(light_dir0));

    if (spot_effect > cos(radians(10))) {
      spot_effect = pow(spot_effect, SPOT_EXP);
      att = spot_effect / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);


      color += att * (n_dot_l * diffuse + ambient); //diffuse * n_dot_l;

      n_dot_hv = max(dot(normal0,half_vector), 0.0);
      color += att * gl_FrontMaterial.specular * specular * pow(n_dot_hv, gl_FrontMaterial.shininess); // gl_FrontMaterial.specular * specular; //

      color.a = 1.0;
    } else {
      color.a = 1.0;
    }
  } else {
    color.a = 1.0;
  }

  gl_FragColor = color;
}