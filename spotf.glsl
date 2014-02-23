#version 120

varying vec3 normal0;

varying vec3 light_dir;

varying vec4  diffuse;
varying vec4  ambient;
varying vec4  specular;

varying vec3 half_vector;

uniform sampler2D texture_color;

void main() {

  float n_dot_l, n_dot_hv, af;
  vec3 cf;
  vec4 color = texture2D(texture_color, gl_TexCoord[0].st);

  n_dot_l = max(dot(normal0, light_dir), 0.0);

  float SHININESS = 0.1;


  //cf = n_dot_l * color.rgb;
  //af = color.a;

  if (n_dot_l > 0.0) {

    color *= n_dot_l * diffuse; //diffuse * n_dot_l;

    n_dot_hv = max(dot(normal0,half_vector), 0.0);
    color += gl_FrontMaterial.specular * specular * pow(n_dot_hv, SHININESS); // gl_FrontMaterial.specular * specular; //
    color.a = 1.0;
    //color.a = 1.0;

    gl_FragColor = color;
  }


  //if (cos_angle > cos(radians(10.0)))
  //gl_FragColor = color;
 // else
 //   gl_FragColor = vec4(cos_angle*cf*0.5, af);
}