#version 120

// ganked from: https://code.google.com/p/opengl-tutorial-org/source/browse/tutorial02_red_triangle/SimpleVertexShader.vertexshader?name=2.1%20branch
//uniform sampler2D texture_color;
/*
void main()
{

  // Output color = red, alpha = 1
  //gl_FragColor = texture2D(texture_color, gl_TexCoord[0].st)// vec4(1,0,0,1);
  gl_FragColor = vec4(1,0,0,1);
}
*/

//uniform sampler2D texture_color;

void main() {
  // Set the output color of our current pixel
  gl_FragColor = vec4(1,1,1,1); // texture2D(texture_color, gl_TexCoord[0].st);
}