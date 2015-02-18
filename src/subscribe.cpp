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
#include <cstdio>

#include "service/subscribe.h"

void sync_subscribe(zmq::socket_t& subscriber, zmq::socket_t& sync_client, int port, int highwater, char type) {
  subscriber.setsockopt(ZMQ_SUBSCRIBE, &type, 1); // subscribe to either 'c'louds or 'p'oses
  if (highwater == 0) {
    int conflate = 1;
    subscriber.setsockopt(ZMQ_CONFLATE, &conflate, sizeof(int)); // keep only last message received
  } else
    subscriber.setsockopt(ZMQ_RCVHWM, &highwater, sizeof(int));

  std::ostringstream subscribe_address, sync_address;

  subscribe_address << "tcp://localhost:" << port << std::flush;
  sync_address      << "tcp://localhost:" << port+1 << std::flush;
    
  std::string subscribe_address_string = subscribe_address.str(),
      sync_address_string = sync_address.str();
    
  subscriber.connect(subscribe_address_string.c_str());
  sync_client.connect(sync_address_string.c_str());

  std::cerr << "Waiting for synchronization (" << port+1 << ")..." << std::flush;

  // Send a synchronization request and wait for a reply from the
  // server.
  char empty_message[] = "";
  zmq::message_t tmp1(static_cast<void*>(empty_message), 0, NULL, NULL), tmp2;
  sync_client.send(tmp1);
  sync_client.recv(&tmp2);

  sync_client.disconnect(sync_address_string.c_str());
    
  std::cerr << "subscribed to " << subscribe_address_string << std::endl;
}


void subscribe(zmq::socket_t& subscriber, int port, int highwater) {
  subscriber.setsockopt(ZMQ_SUBSCRIBE, NULL, 0); // subscribe to all
  if (highwater == 0) {
    int conflate = 1;
    subscriber.setsockopt(ZMQ_CONFLATE, &conflate, sizeof(int)); // keep only last message received
  } else
    subscriber.setsockopt(ZMQ_RCVHWM, &highwater, sizeof(int));

  std::ostringstream address;
  address << "tcp://localhost:" << port << std::flush;
  std::string address_string = address.str();
  
  std::cerr << "Subscribing to " << address_string << "..." << std::flush;
  subscriber.connect(address_string.c_str());
  std::cerr << "done." << std::endl;
}


bool received_shutdown(zmq::message_t& msg) {
  const char KTHXBAI[] = "KTHXBAI";
  char* str = static_cast<char*>(msg.data()) + 1;
  if (msg.size() == 8 && std::strncmp(str, KTHXBAI, 7) == 0) {
    std::cerr << "Received shutdown signal." << std::endl;
    return true;
  }
  return false;
}


recv_result_t receive_poses(zmq::socket_t& subscriber, pose_message_t::ptr& poses, int flags) {
  zmq::message_t message;
  bool result = subscriber.recv(&message, flags);

  // If ZMQ_NOBLOCK is passed in as a flag, we don't actually need to receive any result
  // to report success. The only time we report failure is if a shutdown message is
  // received.
  if ((flags & ZMQ_NOBLOCK) && !result)
    return RECV_NOUPDATE;
  else if (!result)
    return RECV_FAILURE;

  if (received_shutdown(message)) {
    return RECV_SHUTDOWN;

  } else if (pose_message_t::is_pose_message(message)) {
    poses.reset(new pose_message_t(message));
    std::cerr << "Received a set of " << (size_t)(poses->size) << " poses" << std::endl;
    return RECV_SUCCESS;

  } else {
    std::cerr << "Received a message of size " << message.size() << " which doesn't appear to be a pose_message_t; don't know how to continue." << std::endl;
    return RECV_FAILURE;

  }
}


recv_result_t receive_pose(zmq::socket_t& subscriber, Eigen::Matrix4f& pose, unsigned long& timestamp, int flags) {
  zmq::message_t message;
  bool result = subscriber.recv(&message, flags);

  // If ZMQ_NOBLOCK is passed in as a flag, we don't actually need to receive any result
  // to report success. The only time we report failure is if a shutdown message is
  // received.
  if ((flags & ZMQ_NOBLOCK) && !result)
    return RECV_NOUPDATE;
  else if (!result)
    return RECV_FAILURE;

  if ((message.size() - sizeof(unsigned long) - sizeof(char)) / sizeof(float) == 16) {

    std::cerr << "Received a 4x4 matrix" << std::endl;

   void* timestamp_pos = static_cast<void*>(static_cast<char*>(message.data()) + 1);

    memcpy(&timestamp, timestamp_pos, sizeof(unsigned long));
    memcpy(pose.data(), static_cast<void*>(static_cast<char*>(message.data()) + sizeof(unsigned long) + sizeof(char)), sizeof(Eigen::Matrix4f));
    return RECV_SUCCESS;

  } else if (received_shutdown(message)) {
    return RECV_SHUTDOWN;

  } else {
    std::cerr << "Received a message of size " << message.size() << ", expected pose, don't know how to continue." << std::endl;
    return RECV_FAILURE;
  }
}

