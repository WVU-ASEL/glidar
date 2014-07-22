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

#include <Magick++.h>

#include "scene.h"
#include "mesh.h"

using namespace glm;

using std::cerr;
using std::cout;
using std::endl;

const float SPEED = 36.0f;


// Most of main() ganked from here: https://code.google.com/p/opengl-tutorial-org/source/browse/tutorial01_first_window/tutorial01.cpp
int main(int argc, char** argv) {

  std::string model_filename(argc > 1 ? argv[1] : "test.obj");
  float model_scale_factor(argc > 2 ? atof(argv[2]) : 1.0);
  float model_rotate_x(argc > 3 ? atof(argv[3]) : 0.0);
  float model_rotate_y(argc > 4 ? atof(argv[4]) : 0.0);
  float model_rotate_z(argc > 5 ? atof(argv[5]) : 0.0);
  float model_init_rotate_x(argc > 6 ? atof(argv[6]) : 0.0);
  float model_init_rotate_y(argc > 7 ? atof(argv[7]) : 0.0);
  float model_init_rotate_z(argc > 8 ? atof(argv[8]) : 0.0);
  float camera_z(argc > 9 ? atof(argv[9]) : 1000.0);
  unsigned int width(argc > 10 ? atoi(argv[10]) : 256);
  unsigned int height(argc > 11 ? atoi(argv[11]) : 256);
  float fov(argc > 12 ? atof(argv[12]) : 20.0f);
  std::string pcd_filename(argc > 13 ? argv[13] : "");

  std::cerr << "Loading model "      << model_filename << std::endl;
  std::cerr << "Scaling model by "   << model_scale_factor << std::endl;

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

  Scene scene(model_filename, model_scale_factor, camera_z);


  double last_time = 0,
         current_time = glfwGetTime();
  float delta_time = current_time - last_time;

  //Shader shader_program("spotv.glsl", "spotf.glsl");
  Shader shader_program("lidarv.glsl", "lidarf.glsl");

  scene.setup(&shader_program, fov);

  float rx = model_init_rotate_x,
        ry = model_init_rotate_y,
        rz = model_init_rotate_z;

  bool mouse_button_pressed = false;
  bool s_key_pressed = false;

  bool save_and_quit = false;
  bool saved_now_quit= false;

  do {

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
      scene.move_camera(&shader_program, fov, delta_time * SPEED);
    }


    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
      scene.move_camera(&shader_program, fov, delta_time * -SPEED);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      s_key_pressed = true;
    }

    // Write a PCD file if the user presses the 's' key.
    if (save_and_quit || (s_key_pressed && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)) {

      // First, re-render without the box, or it'll show up in our point cloud.
      scene.render(&shader_program, rx, ry, rz, false);

      scene.save_point_cloud(save_and_quit ? pcd_filename : "buffer",
                             width,
                             height);
      scene.save_transformation_metadata(save_and_quit ? pcd_filename : "buffer",
                                         rx, ry, rz);

      s_key_pressed = false;

      if (save_and_quit) saved_now_quit = true;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
      mouse_button_pressed = true;
    }

    if (mouse_button_pressed && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE) {
      mouse_button_pressed = false;
      double mouse_x, mouse_y;
      glfwGetCursorPos(window, &mouse_x, &mouse_y);

      std::cerr << "Click in window at " << mouse_x << ", " << mouse_y << std::endl;

      // First, re-render without the box, or it'll show up in our point cloud.
      scene.render(&shader_program, rx, ry, rz, false);

      glm::dvec3 position = scene.unproject(height, mouse_x, mouse_y);

      std::cerr << "\tcamera z  : " << scene.get_camera_z() << std::endl;
      std::cerr << "\tnear z    : " << scene.get_near_plane() << std::endl;
      std::cerr << "\tfar z     : " << scene.get_far_plane() << std::endl;
      //std::cerr << "\tbuffer val: " << rgba[0] << '\t' << rgba[1] << '\t' << rgba[2] << '\t' << rgba[3] << std::endl;
      std::cerr << "\tcoords    : " << position[0] << '\t' << position[1] << '\t' << position[2] << std::endl;
    }

    rx += model_rotate_x * delta_time;
    ry += model_rotate_y * delta_time;
    rz += model_rotate_z * delta_time;


    scene.render(&shader_program, rx, ry, rz, true); // render with the box

    glfwSwapBuffers(window);
    glfwPollEvents();

    // update timers so we can do camera motion
    last_time = current_time;
    current_time = glfwGetTime();

    save_and_quit = pcd_filename.size() > 0;

  } while (!saved_now_quit && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

  glfwTerminate();
}