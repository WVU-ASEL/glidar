#version 120

varying vec3 normal0;

varying vec3 light_dir;

varying vec4  diffuse;
varying vec4  ambient;
varying vec4  specular;

varying vec3 half_vector;
varying vec3 ec_pos;

uniform sampler2D texture_color;

void main() {

  float n_dot_l, n_dot_hv, att;
  vec3 cf;
  vec4 color = texture2D(texture_color, gl_TexCoord[0].st);
  float spot_effect;
  float SHININESS = 0.001;
  float CATT = 0.1, LATT = 0.1, QATT = 0.1, SPOT_EXP = 500.0;

  vec3 light_dir0 = normalize(vec3(0.0, 0.0, 1000.0) - ec_pos);

  float dist = length(light_dir0);

  n_dot_l = max(dot(normal0, normalize(light_dir0)), 0.0);

  //cf = n_dot_l * color.rgb;
  //af = color.a;

  if (n_dot_l > 0.0) {

    spot_effect = dot(normalize(light_dir), normalize(-light_dir));

    spot_effect = pow(spot_effect, SPOT_EXP);
    att = spot_effect / (CATT + LATT*dist + QATT*dist*dist);


    color *= att * (n_dot_l * diffuse + ambient); //diffuse * n_dot_l;

    n_dot_hv = max(dot(normal0,half_vector), 0.0);
    color += att * gl_FrontMaterial.specular * specular * pow(n_dot_hv, SHININESS); // gl_FrontMaterial.specular * specular; //

    color.a = 1.0;
    //color.a = 1.0;
  } else {
    color.rgb = vec3(0.0, 0.0, 0.0);
    color.a = 1.0;
  }

  gl_FragColor = color;


  //if (cos_angle > cos(radians(10.0)))
  //gl_FragColor = color;
 // else
 //   gl_FragColor = vec4(cos_angle*cf*0.5, af);
}