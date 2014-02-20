// g++ main.cpp -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/projection.hpp>
#include <Magick++.h>

#include "mesh.h"

using namespace glm;

using std::cerr;
using std::cout;
using std::endl;

const GLdouble FOV = 20.0;
const float SPEED = 3.0f;


// Most of main() ganked from here: https://code.google.com/p/opengl-tutorial-org/source/browse/tutorial01_first_window/tutorial01.cpp
int main(int argc, char** argv) {

  std::string model_filename(argv[1]);

  std::cerr << "Loading model " << model_filename << std::endl;

  Magick::InitializeMagick(*argv);

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2); // We want OpenGL 2.1 (latest that will work on my MBA's Intel Sandy Bridge GPU)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We want the OpenGL core profile
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  //std::cerr << "OpenGL version " << glGetString(GL_VERSION) << std::endl;

  // Try to open a window
  GLFWwindow* window = glfwCreateWindow(480, 480, "LIDAR Simulator", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to open GLFW window." << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  // Initialize GLEW
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);

  // Create and compile our GLSL program from the shaders
  //GLuint program_id = load_shaders("simple_transform_shader.glsl", "simple_fragment_shader.glsl");
//  GLuint program_id = load_shaders("phong_vertex.glsl", "simple_fragment_shader.glsl"); //"phong_fragment.glsl");
  Shader shader_program("phong_vertex.glsl", "simple_fragment_shader.glsl");
  GLuint program_id = shader_program.id();

  // Get a handle for our MVP uniform
  GLuint mvp_id = glGetUniformLocation(program_id, "mvp");
  GLuint mv_id  = glGetUniformLocation(program_id, "mv");

  // Get a handle for our buffers
  GLuint vertex_position_model_space_id = glGetAttribLocation(program_id, "vertex_position_model_space");
  GLuint vertex_normal_model_space_id   = glGetAttribLocation(program_id, "N");

  // Projection mtrix with 20d field of view, 1:1 ratio, display range from 0.1 unit to 100 units
  glm::mat4 projection = glm::perspective(90.0f, 1.0f, 0.1f, 500.0f);

  glm::vec3 camera_pos(0,0,100);
  glm::vec4 light_pos(200,150,150,1);

  glm::vec4 specular(1.0f, 1.0f, 1.0f, 1.0f);
  GLfloat shininess[] = { 50.0 };

  glShadeModel(GL_SMOOTH);
  glMaterialfv(GL_FRONT, GL_SPECULAR, &specular[0]);
  glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
  glLightfv(GL_LIGHT0, GL_POSITION, &light_pos[0]);

  // Camera matrix
  glm::mat4 view       = glm::lookAt(
      camera_pos, // Camera is at 4,3,3 in world space
      glm::vec3(0,0,0), // and looks at origin
      glm::vec3(0,1,0)  // head is up (set to 0,-1,0 to look upside-down)
  );

  // Model matrix: an identity matrix, since model is at the origin
  glm::mat4 model     = glm::scale(glm::mat4(1.0), glm::vec3(20.0)); //glm::scale(glm::mat4(1.0), glm::vec3(0.01f));

  // model view projection: multiplication of the three matrices
  glm::mat4 mv        = view * model;
  glm::mat4 mvp       = projection * mv;


  static const GLfloat vertex_buffer_data[] = {
      -0.1, -0.1, 0.0,
       0.1, -0.1, 0.0,
       0.0,  0.1, 0.0
  };

  Mesh* mesh = new Mesh();
  //mesh->load_mesh("ISS models 2011/Scenes/ISS complete_2011.lws");
  mesh->load_mesh(model_filename);

  //static const GLushort element_buffer_data[] = { 0, 1, 2 };

  //GLuint vertex_buffer;

  //glGenBuffers(1, &vertex_buffer);
  //glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  //glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

  double last_time = 0,
         current_time = glfwGetTime();
  float delta_time = current_time - last_time;

  do {

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
      view = glm::translate(view, glm::vec3(0,0,1) * delta_time * SPEED);
      mv = view * model;
      mvp = projection * mv;
      std::cerr << "Camera position: " << camera_pos[0] << '\t' << camera_pos[1] << '\t' << camera_pos[2] << std::endl;
    }

    if (glfwGetKey(window, GLFW_MOD_SHIFT | GLFW_KEY_EQUAL) == GLFW_PRESS) {
      view = glm::translate(view, glm::vec3(0,0,-1) * delta_time * SPEED);
      mv = view * model;
      mvp = projection * mv;
      std::cerr << "Camera position: " << camera_pos[0] << '\t' << camera_pos[1] << '\t' << camera_pos[2] << std::endl;
    }

    // Clear the screen
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    shader_program.bind();


    // Send our transformation to the currently bound shader, in the "MVP" uniform
    glUniformMatrix4fv(mvp_id, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(mv_id, 1, GL_FALSE, glm::value_ptr(mv));

    // 1st attribute buffer: vertices
    //glEnableVertexAttribArray(vertex_position_model_space_id);
    //glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    /*glVertexAttribPointer(
        vertex_position_model_space_id, // attribute we want to configure
        3,  // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0,        // stride
        (void*)0  // array buffer offset
    );*/

    //glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle

    mesh->render(shader_program);

    shader_program.unbind();

    //glDisableVertexAttribArray(vertex_position_model_space_id);

    glfwSwapBuffers(window);
    glfwPollEvents();

    // update timers so we can do camera motion
    last_time = current_time;
    current_time = glfwGetTime();

  } while( glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

  //glDeleteBuffers(1, &vertex_buffer);
  glDeleteProgram(program_id);

  glfwTerminate();
}