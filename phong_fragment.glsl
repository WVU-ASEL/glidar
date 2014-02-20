#version 120

attribute vec3 n;
attribute vec3 vertex_position_model_space;

void main (void)
{
  vec3 l = normalize(gl_LightSource[0].position.xyz - vertex_position_model_space);
  vec3 e = normalize(-vertex_position_model_space); // we are in Eye Coordinates, so EyePos is (0,0,0)
  vec3 r = normalize(-reflect(l, n));

  //calculate Ambient Term:
  vec4 iamb = gl_FrontLightProduct[0].ambient;

  //calculate Diffuse Term:
  vec4 idiff = gl_FrontLightProduct[0].diffuse * max(dot(n,l), 0.0);
  idiff = clamp(idiff, 0.0, 1.0);

  // calculate Specular Term:
  vec4 ispec = gl_FrontLightProduct[0].specular
      * pow(max(dot(r,e),0.0),0.3*gl_FrontMaterial.shininess);
  ispec = clamp(ispec, 0.0, 1.0);
  // write Total Color:
  gl_FragColor = gl_FrontLightModelProduct.sceneColor + iamb + idiff + ispec;
}