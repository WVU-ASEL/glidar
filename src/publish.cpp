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

#include "service/publish.h"

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

void send_shutdown(zmq::socket_t& publisher) {
  const char pBYE[] = "pKTHXBAI";
  zmq::message_t pkthxbai(8);
  memcpy(pkthxbai.data(), pBYE, 8);
  publisher.send(pkthxbai);

  const char PBYE[] = "PKTHXBAI";
  zmq::message_t Pkthxbai(8);
  memcpy(Pkthxbai.data(), PBYE, 8);
  publisher.send(Pkthxbai);

  const char cBYE[] = "cKTHXBAI";
  zmq::message_t ckthxbai(8);
  memcpy(ckthxbai.data(), cBYE, 8);
  publisher.send(ckthxbai);
}


void sync_publish(zmq::socket_t& publisher, zmq::socket_t& sync_service, int port, size_t expected_subscribers, int conflate) {
  if (conflate)
    publisher.setsockopt(ZMQ_CONFLATE, &conflate, sizeof(int)); // keep only last message received
  
  std::ostringstream publish_address, sync_address;

  publish_address << "tcp://*:" << port << std::flush;
  sync_address    << "tcp://*:" << port+1 << std::flush;
  std::string publish_address_string = publish_address.str(),
                 sync_address_string = sync_address.str();

  publisher.bind(publish_address_string.c_str());
  sync_service.bind(sync_address_string.c_str());

  if (expected_subscribers == 1)
    std::cerr << "Waiting for 1 subscriber..." << std::flush;
  else
    std::cerr << "Waiting for " << expected_subscribers << " subscribers..." << std::flush;
  
  char* empty_message = "";
  zmq::message_t tmp2(empty_message, 0, NULL, NULL), tmp1;
  // Wait for synchronization request, then send synchronization
  // reply.

  for (size_t subscribers = 0; subscribers < expected_subscribers; ++subscribers) {
    sync_service.recv(&tmp1);
    sync_service.send(tmp2);
  }

  sync_service.disconnect(sync_address_string.c_str());
  
  std::cerr << "done binding to " << publish_address_string << std::endl;
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


void send_poses(zmq::socket_t& publisher, const pose_message_t::ptr& poses) {
  boost::shared_ptr<zmq::message_t> msg(poses->to_zmq());
  publisher.send(*msg);
}

