// g++ main.cpp -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/projection.hpp>
#include <Magick++.h>

#include "scene.h"

using namespace glm;

using std::cerr;
using std::cout;
using std::endl;

const GLdouble FOV = 20.0;
const float SPEED = 12.0f;


// Most of main() ganked from here: https://code.google.com/p/opengl-tutorial-org/source/browse/tutorial01_first_window/tutorial01.cpp
int main(int argc, char** argv) {

  std::string model_filename(argc > 1 ? argv[1] : "test.obj");
  float model_scale_factor(argc > 2 ? atof(argv[2]) : 100.0);
  float model_rotate_x(argc > 3 ? atof(argv[3]) : 0.0);
  float model_rotate_y(argc > 4 ? atof(argv[4]) : 0.0);
  float model_rotate_z(argc > 5 ? atof(argv[5]) : 0.0);

  std::cerr << "Loading model " << model_filename << std::endl;
  std::cerr << "Scaling model by " << model_scale_factor << std::endl;

  Magick::InitializeMagick(*argv);

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2); // We want OpenGL 2.1 (latest that will work on my MBA's Intel Sandy Bridge GPU)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

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

  Scene scene(model_filename, model_scale_factor);


  double last_time = 0,
         current_time = glfwGetTime();
  float delta_time = current_time - last_time;

  //Shader shader_program("spotv.glsl", "spotf.glsl");
  Shader shader_program("lidarv.glsl", "lidarf.glsl");

  scene.setup(&shader_program);

  float rx = 0.0, ry = 0.0, rz = 0.0;

  do {

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
      scene.move_camera(&shader_program, delta_time * SPEED);
    }


    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
      scene.move_camera(&shader_program, delta_time * -SPEED);
    }

    rx += model_rotate_x * delta_time;
    ry += model_rotate_y * delta_time;
    rz += model_rotate_z * delta_time;


    scene.render(&shader_program, rx, ry, rz);

    glfwSwapBuffers(window);
    glfwPollEvents();

    // update timers so we can do camera motion
    last_time = current_time;
    current_time = glfwGetTime();

  } while( glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

  glfwTerminate();
}