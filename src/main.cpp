/*
 * Copyright (c) 2014 - 2015, John O. Woods, Ph.D.
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
#include "pcl.h"


using std::cerr;
using std::cout;
using std::endl;

const float SPEED = 36.0f;


/*
 * These next few functions are for catching Ctrl+C interrupts. If GLIDAR is killed and has things
 * that are subscribing to it, we want them to be notified that they should end.
 */
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


/** Calls receive_vector in order to obtain a timestamp and 11 floating-point values (the client 
 *  rotation, the translation, and the sensor rotation).
 *
 * @param[in] a subscription socket.
 * @param[out] a timestamp.
 * @param[out] rotation of the client object (the object you're viewing).
 * @param[out] translation between the sensor and the client.
 * @param[out] rotation of the sensor (through which you're viewing).
 * @param[in] flags to pass to the socket (generally 0, which is the default; or ZMQ_NOBLOCK if you want real-time)
 * \returns An enumerator from receive_vector indicating whether a shutdown is in order, there was success, etc.
 */
recv_result_t receive_pose_components(zmq::socket_t& subscriber,
				      timestamp_t& timestamp,
				      glm::dquat& object,
				      glm::dvec3& translation,
				      glm::dquat& sensor,
				      int flags = 0) {
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


/** Main.
 *
 * Sets everything up and then loops --- pretty standard OpenGL --- to intercept keypresses, mouseclicks, to modify the scene,
 * and to redraw.
 *
 * @param[in] Number of arguments from the command line.
 * @param[in] Array of character pointers to the command line arguments.
 *
 * \returns An integer describing the exit status.
 */
int main(int argc, char** argv) {

  // Always expect the first argument to be the 3D model's filename.
  std::string model_filename;
  if (argc < 2) {
    std::cerr << "Error: Expected model filename as first argument." << std::endl;
    exit(-1);
  } else model_filename = argv[1];

  /*
   * 1. Read standard rendering command line arguments.
   */
  float model_scale_factor = 1.0;
  unsigned int width = 256, height = 256;
  float fov = 20.0;
  timestamp_t timestamp = 0;
  std::string pcd_filename;

  glm::dquat object(1.0, 0.0, 0.0, 0.0), sensor(1.0, 0.0, 0.0, 0.0);
  glm::dvec3 object_rotate(0.0, 0.0, 0.0), sensor_rotate(0.0, 0.0, 0.0);
  glm::dvec3 translation(0.0, 0.0, 0.0);

  pcl::console::parse_angle_axis(argc, argv, "--model-r", object);
  pcl::console::parse_quaternion(argc, argv, "--model-q", object);
  pcl::console::parse_3x_arguments(argc, argv, "--model-dr", object_rotate);
  pcl::console::parse_angle_axis(argc, argv, "--camera-r", sensor);
  pcl::console::parse_quaternion(argc, argv, "--camera-q", sensor);
  pcl::console::parse_3x_arguments(argc, argv, "--camera-dr", sensor_rotate);

  std::cerr << "Read in model rotation of " << object[0] << ", " << object[1] << ", " << object[2] << ", " << object[3] << std::endl;
  std::cerr << "Read in camera rotation of " << sensor[0] << ", " << sensor[1] << ", " << sensor[2] << ", " << sensor[3] << std::endl;

  pcl::console::parse(argc, argv, "--scale", model_scale_factor);
  pcl::console::parse(argc, argv, "--camera-z", translation[2]);

  pcl::console::parse(argc, argv, "--fov", fov);
  pcl::console::parse(argc, argv, "--pcd", pcd_filename);

  pcl::console::parse(argc, argv, "--width", width);
  pcl::console::parse(argc, argv, "-w", width);
  pcl::console::parse(argc, argv, "--height", height);
  pcl::console::parse(argc, argv, "-h", height);

  /*
   * 2. If ZeroQ is included, let's allow GLIDAR to be connected to a loop and send and receive data. Read those command line arguments.
   */
  int port = 0, physics_port = 0;
  int frequency = 15;
  int subscribers = 1;
  int highwater_mark = 0;
  int conflate = 0;
  pcl::console::parse(argc, argv, "--port", port);
  pcl::console::parse(argc, argv, "--physics-port", physics_port);
  pcl::console::parse(argc, argv, "-p", port);
  pcl::console::parse(argc, argv, "--subscribers", subscribers);
  pcl::console::parse(argc, argv, "--pub-rate", frequency);
  pcl::console::parse(argc, argv, "--hwm", highwater_mark);
  pcl::console::parse(argc, argv, "--pub-conflate", conflate);

  zmq::context_t context(1);
  zmq::socket_t publisher(context, ZMQ_PUB);
  zmq::socket_t subscriber(context, ZMQ_SUB);
  zmq::socket_t truth_publisher(context, ZMQ_PUB);
  zmq::socket_t sync_service(context, ZMQ_REP);
  zmq::socket_t sync_client(context, ZMQ_REQ);
  //PoseLogger logger("sensor.pose");

  /*
   * 3. Use ZeroMQ to publish and subscribe, both functions waiting for synchronization before allowing us to proceed forward.
   */
  if (port)
    sync_publish(publisher, sync_service, port, subscribers, conflate);

  if (physics_port)
    sync_subscribe(subscriber, sync_client, physics_port, highwater_mark, 'v');

  /* This is where we start to catch signals. */
  s_catch_signals ();

  std::cerr << "Loading model "      << model_filename << std::endl;
  std::cerr << "Scaling model by "   << model_scale_factor << std::endl;

  /* Send along command line arguments to ImageMagick in case it has anything it needs to process. Never used this; haven't tested it. */
  Magick::InitializeMagick(*argv);

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2); // We want OpenGL 2.1 (latest that will work on my MBA's Intel Sandy Bridge GPU)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  // std::cerr << "OpenGL version " << glGetString(GL_VERSION) << std::endl;

  /*
   * 4. Attempt to create a window that we can draw our LIDAR images in.
   */
  GLFWwindow* window = glfwCreateWindow(width, height, "GLIDAR", NULL, NULL);
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

  // Ensure we can capture keypresses.
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  Scene scene(model_filename, model_scale_factor, -translation[2]);


  double last_time = 0,
         current_time = glfwGetTime();
  float delta_time = current_time - last_time;

  Shader shader_program("shaders/spotv.glsl", "shaders/lidarf.glsl");

  bool s_key_pressed = false;

  bool save_and_quit = false;
  bool saved_now_quit= false;
  

  size_t loopcount = 0;
  unsigned short backspaces = 0;
  timestamp_t last_timestamp_sent = 0;
 
  std::cerr << "Maximum buffer size: " << width * height * 4 * sizeof(float) << std::endl;

  /*
   * 5. Main event loop.
   */
  do {

    /*
     * If we're not using a physics simulator, we have limited options.
     *
     * * The '-' key moves the camera forward.
     * * The '+' key moves the camera backwards.
     * * The 's' key spits out a PCD file containing a point cloud representation of the current view.
     *   That will go in buffer.pcd most of the time.
     * * If you specified an output filename using --pcd, it's going to basically press the 's' key for you
     *   and then exit.
     *
     * Each of the following if-statements records the key-press. I'm not 100% sure what happens if you hold 
     * down 's', but you should be able to hold down + and - to move steadily.
     */
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
      translation[2] -= delta_time * SPEED;
    }


    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
      translation[2] += delta_time * SPEED;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      s_key_pressed = true;
    }

    /*
     * If we are using a physics simulator, we want to receive updated pose information from it each time
     * we iterate through the loop. It might also provide us with a shutdown message, so we need to process
     * that as well.
     */
    recv_result_t receive_result;
    if (physics_port) {
      receive_result = receive_pose_components(subscriber, timestamp, object, translation, sensor);
      if (receive_result == RECV_SHUTDOWN) s_interrupted = true;
      
/*      std::cerr << "Physics instructing a render with the following:\n";
      std::cerr << "  object:\t" << to_string(object) << std::endl;
      std::cerr << "  transl:\t" << glm::to_string(translation) << std::endl;
      std::cerr << "  sensor:\t" << to_string(sensor) << std::endl; */
    }

    /*
     * Handle key-releases, or the case where the user provided a --pcd file output (then we'll exist after
     * the first loop iteration.
     */
    if (save_and_quit || (s_key_pressed && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)) {

      // Physics simulator: Render before we try to save. Then save the point cloud and transformation information.
      // I think this is for the case where you might want to use your physics simulator to run a Monte Carlo, as
      // it appears to quit after rendering.

      scene.render(&shader_program, fov, object, translation, sensor);
      scene.save_point_cloud(save_and_quit ? pcd_filename : "buffer", width, height);
      scene.save_transformation_metadata(save_and_quit ? pcd_filename : "buffer", object, translation, sensor);

      s_key_pressed = false;

      if (save_and_quit) saved_now_quit = true;
    }

    // No Physics simulator: alter scene according to command line arguments.
    if (!physics_port) {
      
      object = quaternion_change(object, object_rotate, delta_time);
      sensor = quaternion_change(sensor, sensor_rotate, delta_time);
	
      std::cerr << "Command line instructing a render with the following:\n";
      std::cerr << "  object:\t" << to_string(object) << std::endl;
      std::cerr << "  transl:\t" << glm::to_string(translation) << std::endl;
      std::cerr << "  sensor:\t" << to_string(sensor) << std::endl;
    }
    // If physics simulator is given, we'll just alter based on what we get from physics.


    // Render regardless.
    scene.render(&shader_program, fov, object, translation, sensor);


    /*
     * If we're publishing, we should send the point cloud every few loop iterations.
     *
     * TODO: Point cloud publishing code needs to go in its own function, as this is cluttery.
     */
    if (loopcount == frequency && port) {
      if (!physics_port) ++timestamp; // Need a timestamp for when we're not getting one from physics.

      // Make sure we don't send data, even slightly different data, with the same timestamp. Each timestamp should have
      // one unique point cloud.
      if (timestamp != last_timestamp_sent) {
	// Now indicate that we're sending a point cloud
	const char TYPE = 'c';

	void* send_buffer = malloc(sizeof(char) + sizeof(unsigned long) + width*height*sizeof(float)*4);
	void* timestamp_buffer = static_cast<float*>(static_cast<void*>(static_cast<char*>(send_buffer) + sizeof(char)));
	float* cloud_buffer = static_cast<float*>(static_cast<void*>(static_cast<char*>(send_buffer) + sizeof(unsigned long) + sizeof(char)));

	size_t cloud_size = scene.write_point_cloud(cloud_buffer, width, height);
      
	size_t send_buffer_size = sizeof(unsigned long) + sizeof(char) +
	  cloud_size * sizeof(float);
	memcpy(send_buffer, &TYPE, sizeof(char));
	memcpy(timestamp_buffer, &timestamp, sizeof(unsigned long));
	zmq::message_t message(send_buffer, send_buffer_size, c_message_free, NULL);
	publisher.send(message);
	last_timestamp_sent = timestamp;

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
    }

    /*
     * If the user hits Ctrl+C or ESC, let's indicate that we're shutting down (to any subscribers) and then
     * exit without bothering to save anything.
     */
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

    // update timers so we can do camera motion (non-physics simulator version)
    last_time = current_time;
    current_time = glfwGetTime();

    save_and_quit = pcd_filename.size() > 0;

    ++loopcount;
  
  } while (!saved_now_quit);

  // Close the window.
  glfwTerminate();

  // Success!
  return 0;
}
