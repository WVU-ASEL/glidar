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

#include <pcl/console/parse.h>

#include "service/publish.h"
#include "service/subscribe.h"
#include "scene.h"
#include "mesh.h"
//#include "service/pose_logger.h"


using std::cerr;
using std::cout;
using std::endl;

const float SPEED = 36.0f;

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


recv_result_t receive_model_view_matrices(zmq::socket_t& subscriber, Eigen::Matrix4f& inverse_model, Eigen::Matrix4f& view, int flags = 0) {
  boost::shared_ptr<pose_message_t> poses;
  recv_result_t r = receive_poses(subscriber, poses, flags);
  if (r == RECV_SUCCESS) {
    memcpy(inverse_model.data(), &((*poses)[0].pose), sizeof(float)*16);
    memcpy(view.data(),          &((*poses)[1].pose), sizeof(float)*16);
  }

  std::cerr << "Receive model:" << inverse_model << std::endl;
  std::cerr << "Receive view:" << view << std::endl;
  return r;
}


recv_result_t receive_model_view_matrices(zmq::socket_t& subscriber, glm::mat4& inverse_model, glm::mat4& view, int flags = 0) {
  boost::shared_ptr<pose_message_t> poses;
  recv_result_t r = receive_poses(subscriber, poses, flags);
  if (r == RECV_SUCCESS) {
    memcpy(glm::value_ptr(inverse_model), &((*poses)[0].pose), sizeof(float)*16);
    memcpy(glm::value_ptr(view),          &((*poses)[1].pose), sizeof(float)*16);
  }

  std::cerr << "view: " << glm::to_string(view) << std::endl;
  return r;
}


recv_result_t receive_pose_components(zmq::socket_t& subscriber, timestamp_t& timestamp, glm::dquat& object, glm::dvec3& translation, glm::dquat& sensor, int flags = 0) {
  std::vector<double> v;
  recv_result_t r = receive_vector(subscriber, timestamp, v, flags);
  if (r == RECV_SUCCESS) {
    object.w = v[0];
    object.x = v[1];
    object.y = v[2];
    object.z = v[3];
    translation.x = v[4];
    translation.y = v[5];
    translation.z = v[6];
    sensor.w = v[7];
    sensor.x = v[8];
    sensor.y = v[9];
    sensor.z = v[10];
  }

  return r;
}


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
  float camera_rotate_x = 0.0, camera_rotate_y = 0.0, camera_rotate_z = 0.0;
  float model_init_rotate_x = 0.0, model_init_rotate_y = 0.0, model_init_rotate_z = 0.0;
  float camera_init_rotate_x = 0.0, camera_init_rotate_y = 0.0, camera_init_rotate_z = 0.0;
  float camera_x = 0.0, camera_y = 0.0, camera_z = 1000.0;
  unsigned int width = 256, height = 256;
  float fov = 20.0;
  timestamp_t timestamp = 0;
  std::string pcd_filename;

  pcl::console::parse(argc, argv, "--scale", model_scale_factor);
  pcl::console::parse_3x_arguments(argc, argv, "--model-r", model_init_rotate_x, model_init_rotate_y, model_init_rotate_z);
  pcl::console::parse_3x_arguments(argc, argv, "--model-dr", model_rotate_x, model_rotate_y, model_rotate_z);
  pcl::console::parse(argc, argv, "--camera-z", camera_z);
  pcl::console::parse_3x_arguments(argc, argv, "--camera-xyz", camera_x, camera_y, camera_z);
  pcl::console::parse_3x_arguments(argc, argv, "--camera-r", camera_init_rotate_x, camera_init_rotate_y, camera_init_rotate_z);
  pcl::console::parse_3x_arguments(argc, argv, "--camera-dr", camera_rotate_x, camera_rotate_y, camera_rotate_z);
  pcl::console::parse(argc, argv, "--fov", fov);
  pcl::console::parse(argc, argv, "--pcd", pcd_filename);

  pcl::console::parse(argc, argv, "--width", width);
  pcl::console::parse(argc, argv, "-w", width);
  pcl::console::parse(argc, argv, "--height", height);
  pcl::console::parse(argc, argv, "-h", height);

  /*
   * If ZeroMQ is included, let's publish the data.
   */
  int port = 0, physics_port = 0;
  int frequency = 15;
  int subscribers = 1;
  int highwater_mark = 0;
  pcl::console::parse(argc, argv, "--port", port);
  pcl::console::parse(argc, argv, "--physics-port", physics_port);
  pcl::console::parse(argc, argv, "-p", port);
  pcl::console::parse(argc, argv, "--subscribers", subscribers);
  pcl::console::parse(argc, argv, "--pub-rate", frequency);
  pcl::console::parse(argc, argv, "--hwm", highwater_mark);

  zmq::context_t context(1);
  zmq::socket_t publisher(context, ZMQ_PUB);
  zmq::socket_t subscriber(context, ZMQ_SUB);
  zmq::socket_t truth_publisher(context, ZMQ_PUB);
  zmq::socket_t sync_service(context, ZMQ_REP);
  zmq::socket_t sync_client(context, ZMQ_REQ);
  //PoseLogger logger("sensor.pose");
    
  if (port)
    sync_publish(publisher, sync_service, port, subscribers);

  if (physics_port)
    sync_subscribe(subscriber, sync_client, physics_port, highwater_mark, 'v');
  
  s_catch_signals ();

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
  
  float mrx = model_init_rotate_x,
        mry = model_init_rotate_y,
        mrz = model_init_rotate_z,
    crx = camera_init_rotate_x,
    cry = camera_init_rotate_y,
    crz = camera_init_rotate_z;

  //scene.projection_setup(fov, mrx, mry, mrz, camera_x, camera_y, camera_z, crx, cry, crz);

  bool mouse_button_pressed = false;
  bool s_key_pressed = false;

  bool save_and_quit = false;
  bool saved_now_quit= false;
  

  size_t loopcount = 0;
  unsigned short backspaces = 0;
 
  std::cerr << "Maximum buffer size: " << width * height * 4 * sizeof(float) << std::endl;
  
  do {

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
      camera_z -= delta_time * SPEED;
    }


    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
      camera_z += delta_time * SPEED;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      s_key_pressed = true;
    }

    recv_result_t receive_result;
    glm::dquat object, sensor;
    glm::dvec3 translation;
    if (physics_port) {
      receive_result = receive_pose_components(subscriber, timestamp, object, translation, sensor);
      //receive_result = receive_model_view_matrices(subscriber, inverse_model, view);
      if (receive_result == RECV_SHUTDOWN) s_interrupted = true;
    }

    // Write a PCD file if the user presses the 's' key.
    if (save_and_quit || (s_key_pressed && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)) {

      // First, re-render without the box, or it'll show up in our point cloud.
      if (physics_port) {
	scene.render(&shader_program, fov, object, translation, sensor);
	scene.save_point_cloud(object, translation, sensor, save_and_quit ? pcd_filename : "buffer", width, height);
	scene.save_transformation_metadata(save_and_quit ? pcd_filename : "buffer", object, translation, sensor);
      } else {
        scene.render(&shader_program, fov, mrx, mry, mrz, camera_x, camera_y, camera_z, crx, cry, crz);

        scene.save_point_cloud(mrx, mry, mrz,
			     camera_x, camera_y, camera_z,
			     crx, cry, crz,
			     save_and_quit ? pcd_filename : "buffer",
                             width,
                             height);
        scene.save_transformation_metadata(save_and_quit ? pcd_filename : "buffer",
                                         mrx, mry, mrz,
					 camera_x, camera_y, camera_z,
					 crx, cry, crz);
      }

      s_key_pressed = false;

      if (save_and_quit) saved_now_quit = true;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
      mouse_button_pressed = true;
    }

    if (physics_port) {
      //Eigen::Vector3f angle = pose.topLeftCorner<3,3>().eulerAngles(0,1,2);
      //scene.move_camera_to(&shader_program, pose(0,3), pose(1,3), pose(2,3));
      //scene.render(&shader_program, fov, rx, ry, rz, angle[0] * 180.0/M_PI, angle[1] * 180.0/M_PI, angle[2] * 180.0/M_PI, false);
      scene.render(&shader_program, fov, object, translation, sensor);
      
    } else {
      mrx += model_rotate_x * delta_time;
      mry += model_rotate_y * delta_time;
      mrz += model_rotate_z * delta_time;

      crx += camera_rotate_x * delta_time;
      cry += camera_rotate_y * delta_time;
      crz += camera_rotate_z * delta_time;
      
      scene.render(&shader_program, fov, mrx, mry, mrz, camera_x, camera_y, camera_z, crx, cry, crz); // render without the box
    }

    if (loopcount == frequency && port) {
      if (!physics_port) ++timestamp;

      //if (!physics_port)
        //pose = scene.get_pose(rx, ry, rz, crx, cry, crz);

      // Write timestamp and pose to the logger.
      //      logger.log(timestamp, pose);

      // Transmit the true pose.
      //      send_pose(publisher, pose, timestamp);

      // Now indicate that we're sending a point cloud
      const char TYPE = 'c';

      void* send_buffer = malloc(sizeof(char) + sizeof(unsigned long) + width*height*sizeof(float)*4);
      void* timestamp_buffer = static_cast<float*>(static_cast<void*>(static_cast<char*>(send_buffer) + sizeof(char)));
      float* cloud_buffer = static_cast<float*>(static_cast<void*>(static_cast<char*>(send_buffer) + sizeof(unsigned long) + sizeof(char)));

      size_t cloud_size = physics_port ?
	scene.write_point_cloud(object, translation, sensor, cloud_buffer, width, height) : 
	scene.write_point_cloud(mrx, mry, mrz, camera_x, camera_y, camera_z, crx, cry, crz, cloud_buffer, width, height);
      
      size_t send_buffer_size = sizeof(unsigned long) + sizeof(char) +
	cloud_size * sizeof(float);
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

    glfwSwapBuffers(window);
    glfwPollEvents();

    // update timers so we can do camera motion
    last_time = current_time;
    current_time = glfwGetTime();

    save_and_quit = pcd_filename.size() > 0;

    ++loopcount;
  
  } while (!saved_now_quit);

  glfwTerminate();

  return 0;
}
