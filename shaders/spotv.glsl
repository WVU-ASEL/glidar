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

attribute vec3 position;
attribute vec2 diffuse_tex;
attribute vec2 specular_tex;
attribute vec3 normal;

uniform mat4 LightModelViewMatrix;

varying vec3 normal0;

varying vec4  diffuse;
varying vec4  ambient;
varying vec4  specular;
varying vec3  half_vector;
varying vec3  ec_pos;

void main() {
  specular = vec4(1.0, 1.0, 1.0, 1.0);

  normal0 = normalize(gl_NormalMatrix * normal);

  // Get coordinates in camera frame.
  ec_pos = vec3(gl_ModelViewMatrix * vec4(position, 1.0));
  vec3 ec_light_dir = vec3(LightModelViewMatrix * vec4(gl_LightSource[0].spotDirection, 0.0));

  // Normally in a shading model, we use the half_vector to deal with the difference between
  // the light and the viewer. For most 3D sensors, however, we assume the light source and
  // the viewer are in the same location.
  half_vector = normalize(ec_light_dir);
  //half_vector = normalize(ec_light_dir + ec_light_dir);

  gl_TexCoord[0].st = diffuse_tex;
  gl_TexCoord[1].st = specular_tex;

  diffuse  = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
  specular = gl_FrontMaterial.specular * gl_LightSource[0].specular;
  ambient  = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;

  gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
}