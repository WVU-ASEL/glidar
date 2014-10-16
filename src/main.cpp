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
#include <sstream>
#include <csignal>

#include <Magick++.h>

#include <pcl/console/parse.h>

#ifdef HAS_ZEROMQ
# include <zmq.hpp>
# include <Eigen/Dense>

void cpp_message_free(float* data, void* hint) {
  delete data;
}


extern "C" {
  void message_free(void* data, void* hint) {
    cpp_message_free(static_cast<float*>(data), hint);
  }

  void c_message_free(void* data, void* hint) {
    free(data);
  }
}

void send_pose(zmq::socket_t& publisher, const Eigen::Matrix4f& pose, const unsigned long& timestamp) {
  size_t size = sizeof(char) + sizeof(unsigned long) + 16 * sizeof(float);
  void* send_buffer = malloc(size);
  void* timestamp_buffer = static_cast<void*>(static_cast<char*>(send_buffer) + 1);
  void* pose_buffer = static_cast<void*>(static_cast<char*>(send_buffer) + sizeof(unsigned long) + 1);

  const char TYPE = 'p';

  memcpy(send_buffer, &TYPE, sizeof(char));
  memcpy(timestamp_buffer, &timestamp, sizeof(unsigned long));
  memcpy(pose_buffer, pose.data(), 16 * sizeof(float));

  zmq::message_t message(send_buffer, size, c_message_free, NULL);
  publisher.send(message);
}

#endif

#include "scene.h"
#include "mesh.h"
#include "service/pose_logger.h"

using namespace glm;

using std::cerr;
using std::cout;
using std::endl;

const float SPEED = 36.0f;


#ifdef HAS_ZEROMQ
#include <csignal>

static int s_interrupted = 0;
static void s_signal_handler(int signal_value) {
  s_interrupted = 1;
}
static void s_catch_signals(void) {
  struct sigaction action;
  action.sa_handler = s_signal_handler;
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
}

void sync_publish(zmq::socket_t& publisher, zmq::socket_t& sync_service, int port, size_t expected_subscribers = 1) {
  std::ostringstream publish_address, sync_address;

  publish_address << "tcp://*:" << port << std::flush;
  sync_address    << "tcp://*:" << port+1 << std::flush;
  std::string publish_address_string = publish_address.str(),
                 sync_address_string = sync_address.str();
    
  publisher.bind(publish_address_string.c_str());
  sync_service.bind(sync_address_string.c_str());

  if (expected_subscribers == 1)
    std::cerr << "Waiting for 1 subscriber (" << port+1 << ")..." << std::flush;
  else
    std::cerr << "Waiting for " << expected_subscribers << " subscribers (" << port+1 << ")..." << std::flush;
  
  char* empty_message = "";
  zmq::message_t tmp2(empty_message, 0, NULL, NULL), tmp1;
  // Wait for synchronization request, then send synchronization
  // reply.

  for (size_t subscribers = 0; subscribers < expected_subscribers; ++subscribers) {
    sync_service.recv(&tmp1);
    std::cerr << 'r';
    sync_service.send(tmp2);
    std::cerr << 's';
  }

  sync_service.disconnect(sync_address_string.c_str());
  
  std::cerr << "done binding to " << publish_address_string << std::endl;
}

void send_shutdown(zmq::socket_t& publisher) {
  const char pBYE[] = "pKTHXBAI";
  zmq::message_t pkthxbai(8);
  memcpy(pkthxbai.data(), pBYE, 8);
  publisher.send(pkthxbai);

  const char cBYE[] = "cKTHXBAI";
  zmq::message_t ckthxbai(8);
  memcpy(ckthxbai.data(), cBYE, 8);
  publisher.send(ckthxbai);
}

#endif


// Most of main() ganked from here: https://code.google.com/p/opengl-tutorial-org/source/browse/tutorial01_first_window/tutorial01.cpp
int main(int argc, char** argv) {

  // Always require that the model filename be given.
  std::string model_filename;
  if (argc < 2) {
    std::cerr << "Error: Expected model filename as first argument." << std::endl;
    exit(-1);
  } else model_filename = argv[1];

  float model_scale_factor = 1.0;
  float model_rotate_x = 0.0, model_rotate_y = 0.0, model_rotate_z = 0.0;
  float model_init_rotate_x = 0.0, model_init_rotate_y = 0.0, model_init_rotate_z = 0.0;
  float camera_z = 1000.0;
  unsigned int width = 256, height = 256;
  float fov = 20.0;
  unsigned long timestamp = 0;
  std::string pcd_filename;

  pcl::console::parse(argc, argv, "--scale", model_scale_factor);
  pcl::console::parse_3x_arguments(argc, argv, "--r", model_init_rotate_x, model_init_rotate_y, model_init_rotate_z);
  pcl::console::parse_3x_arguments(argc, argv, "--dr", model_rotate_x, model_rotate_y, model_rotate_z);
  pcl::console::parse(argc, argv, "--camera-z", camera_z);
  pcl::console::parse(argc, argv, "--fov", fov);
  pcl::console::parse(argc, argv, "--pcd", pcd_filename);

  pcl::console::parse(argc, argv, "--width", width);
  pcl::console::parse(argc, argv, "-w", width);
  pcl::console::parse(argc, argv, "--height", height);
  pcl::console::parse(argc, argv, "-h", height);

  /*
   * If ZeroMQ is included, let's publish the data.
   */
#ifdef HAS_ZEROMQ
  int port = 0;
  int frequency = 15;
  int subscribers = 1;
  pcl::console::parse(argc, argv, "--port", port);
  pcl::console::parse(argc, argv, "-p", port);
  pcl::console::parse(argc, argv, "--subscribers", subscribers);
  pcl::console::parse(argc, argv, "--pub-rate", frequency);

  zmq::context_t context(1);
  zmq::socket_t publisher(context, ZMQ_PUB);
  zmq::socket_t truth_publisher(context, ZMQ_PUB);
  zmq::socket_t sync_service(context, ZMQ_REP);
  PoseLogger logger("sensor.pose");
    
  if (port)
    sync_publish(publisher, sync_service, port, subscribers);
  
  s_catch_signals ();
#endif

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

  Shader shader_program("shaders/spotv.glsl", "shaders/lidarf.glsl");
  //Shader shader_program("lidarv.glsl", "lidarf.glsl");

  float rx = model_init_rotate_x,
        ry = model_init_rotate_y,
        rz = model_init_rotate_z;

  scene.projection_setup(fov, rx, ry, rz);

  bool mouse_button_pressed = false;
  bool s_key_pressed = false;

  bool save_and_quit = false;
  bool saved_now_quit= false;
  

  size_t loopcount = 0;
  unsigned short backspaces = 0;
 
  std::cerr << "Maximum buffer size: " << width * height * 4 * sizeof(float) << std::endl;
  
  do {

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
      scene.move_camera(&shader_program, delta_time * SPEED);
    }


    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
      scene.move_camera(&shader_program, delta_time * -SPEED);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      s_key_pressed = true;
    }

    // Write a PCD file if the user presses the 's' key.
    if (save_and_quit || (s_key_pressed && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)) {

      // First, re-render without the box, or it'll show up in our point cloud.
      scene.render(&shader_program, fov, rx, ry, rz, false);

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



#ifdef HAS_ZEROMQ
    scene.render(&shader_program, fov, rx, ry, rz, false); // render without the box

    if (loopcount == frequency && port) {
      ++timestamp;

      Eigen::Matrix4f pose = scene.get_pose(rx, ry, rz);

      // Write timestamp and pose to the logger.
      logger.log(timestamp, pose);

      // Transmit the true pose.
      send_pose(publisher, pose, timestamp);

      // Now indicate that we're sending a point cloud
      const char TYPE = 'c';

      void* send_buffer = malloc(sizeof(char) + sizeof(unsigned long) + width*height*sizeof(float)*4);
      void* timestamp_buffer = static_cast<float*>(static_cast<void*>(static_cast<char*>(send_buffer) + sizeof(char)));
      float* cloud_buffer = static_cast<float*>(static_cast<void*>(static_cast<char*>(send_buffer) + sizeof(unsigned long) + sizeof(char)));
      size_t send_buffer_size = sizeof(unsigned long) + sizeof(char) + scene.write_point_cloud(cloud_buffer, width, height) * sizeof(float);
      memcpy(send_buffer, &TYPE, sizeof(char));
      memcpy(timestamp_buffer, &timestamp, sizeof(unsigned long));
      zmq::message_t message(send_buffer, send_buffer_size, c_message_free, NULL);
      publisher.send(message);

      std::ostringstream length_stream;
      length_stream << send_buffer_size;
      std::string length = length_stream.str();

      // Delete the old length
      for (unsigned short b = 0; b < backspaces; ++b)
	std::cerr << '\b';
      std::cerr << length;

      loopcount = 0;
      backspaces = length.size();      
    }

    if (s_interrupted || glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwWindowShouldClose(window)) {
      if (port > 0) {
	std::cerr << "Interrupt received, sending shutdown signal..." << std::flush;
	send_shutdown(publisher);
	std::cerr << "Done." << std::endl;
      }
      saved_now_quit = true;
    }
#else
    scene.render(&shader_program, fov, rx, ry, rz, true); // render with the box
#endif

    glfwSwapBuffers(window);
    glfwPollEvents();

    // update timers so we can do camera motion
    last_time = current_time;
    current_time = glfwGetTime();

    save_and_quit = pcd_filename.size() > 0;

    ++loopcount;

#ifdef HAS_ZEROMQ    
  } while (!saved_now_quit);
#else
  } while (!saved_now_quit && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
#endif

  glfwTerminate();

  return 0;
}
