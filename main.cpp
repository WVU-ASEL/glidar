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
  unsigned int width(argc > 6 ? atoi(argv[6]) : 256);
  unsigned int height(argc > 7 ? atoi(argv[7]) : 256);

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
  GLFWwindow* window = glfwCreateWindow(width, height, "LIDAR Simulator", NULL, NULL);
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

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      //scene.save_point_cloud("buffer.pcd", width, height);
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
      double mouse_x, mouse_y;
      glfwGetCursorPos(window, &mouse_x, &mouse_y);

      std::cerr << "Click in window at " << mouse_x << ", " << mouse_y << std::endl;

      /*// Transform into 3d normalized device coordinates. Do this by scaling into [-1:1] for x, y, z.
      float x = (2.0f * mouse_x) / float(width) - 1.0f;
      float y = 1.0f - (2.0f * mouse_y) / height;
      float z = 1.0f; // z is not actually needed yet.
      glm::vec3 normalized_device_coordinates(x, y, z);

      // Transform to 4D homogeneous clip coordinates. Our ray's z should point forwards (which is
      // the negative z direction). w = 1 for the 4D vector (instead of point).
      glm::vec4 clip_coordinates(normalized_device_coordinates.xy, -1.0, 1.0); */
      glm::ivec4 viewport;
      glm::dmat4 model_view_matrix, projection_matrix;
      glGetDoublev( GL_MODELVIEW_MATRIX, (double*)&model_view_matrix );
      glGetDoublev( GL_PROJECTION_MATRIX, (double*)&projection_matrix );
      glGetIntegerv( GL_VIEWPORT, (int*)&viewport );

      // Get near and far plane points
      glm::dvec3 near, far;
      gluUnProject(mouse_x, mouse_y, 0.0, (double*)&model_view_matrix, (double*)&projection_matrix, (int*)&viewport, &(near[0]), &(near[1]), &(near[2]));
      gluUnProject(mouse_x, mouse_y, 1.0, (double*)&model_view_matrix, (double*)&projection_matrix, (int*)&viewport, &(far[0]), &(far[1]), &(far[2]));
      glm::dvec3 relative_far = near - far;
      glm::vec4 rgba;
      glReadPixels(mouse_x, height - mouse_y, 1, 1, GL_RGBA, GL_FLOAT, (float*)&rgba);

      double t = rgba[2];
      std::cerr << "\tt = " << t << std::endl;

      std::cerr << "\tnear plane: " << near[0] << '\t' << near[1] << '\t' << near[2] << std::endl;
      std::cerr << "\tfar plane : " << far[0]  << '\t' << far[1]  << '\t' << far[2]  << std::endl;
      std::cerr << "\ttrans far : " << relative_far[0] << '\t' << relative_far[1]  << '\t' << relative_far[2]  << std::endl;
      std::cerr << "\tbuffer val: " << rgba[0] << '\t' << rgba[1] << '\t' << rgba[2] << '\t' << rgba[3] << std::endl;

      glm::dvec3 position = relative_far * t;
      position[0] = -position[0]; // invert x

      std::cerr << "\tcoords    : " << position[0] << '\t' << position[1] << '\t' << position[2] << std::endl;
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