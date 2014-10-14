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

#ifndef SERVICE_H
# define SERVICE_H

typedef unsigned long timestamp_t;
typedef long timestamp_diff_t;             // WARNING: Somewhat fragile. Probably safe since timestamps will never differ by so much.
typedef long long long_signed_timestamp_t; // Safer than the above; use for conversions.

#include <limits>
#include <Eigen/Dense>

struct pose_output_t {
  typedef float score_t;

  Eigen::Matrix4f pose;
  bool converged;
  score_t score;

  pose_output_t()
  : pose(Eigen::Matrix4f::Identity()),
    converged(false),
    score(std::numeric_limits<score_t>::infinity())
  { }
};

#include <vector>
#include <zmq.hpp>
#include <boost/shared_ptr.hpp>

struct pose_message_t {
  typedef unsigned char short_size_t;
  typedef std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > transform_vector_t;
  typedef boost::shared_ptr<pose_message_t> ptr;

  timestamp_t timestamp;
  short_size_t size;
  //transform_vector_t* data;
  pose_output_t* data;

  static bool is_pose_message(const zmq::message_t& msg) {
    return static_cast<const char*>(msg.data())[0] == 'P';
  }

  pose_message_t(size_t sz = 1, timestamp_t t = 0)
  : timestamp(t),
    size((short_size_t)(sz)),
    data(new pose_output_t[sz])
  { }

  pose_message_t(timestamp_t t, const transform_vector_t& transforms)
  : timestamp(t),
    size((short_size_t)(transforms.size())),
    data(new pose_output_t[size])
  {
    for (short_size_t i = 0; i < size; ++i) {
      data[i].pose = transforms[i];
    }
  }

  ~pose_message_t() {
    delete [] data;
  }

  pose_message_t(const zmq::message_t& msg) {
    // get the timestamp and size
    memcpy(static_cast<void*>(this),
	   static_cast<const void*>(static_cast<const char*>(msg.data()) + 1), // ignore type indicator, so start at +1
	   sizeof(short_size_t) + sizeof(timestamp_t));

    data = new pose_output_t[size];

    // Figure out the start point for the matrices
    const pose_output_t* p = static_cast<const pose_output_t*>(static_cast<const void*>(static_cast<const char*>(msg.data()) + 1 + sizeof(short_size_t) + sizeof(timestamp_t)));

    // Copy the matrices
    for (short_size_t i = 0; i < size; ++i) {
      data[i] = p[i];
    }
  }

  boost::shared_ptr<zmq::message_t> to_zmq() {
    boost::shared_ptr<zmq::message_t> msg(new zmq::message_t(sizeof(char) + sizeof(timestamp) + sizeof(size) + sizeof(pose_output_t) * size));

    const char TYPE = 'P'; // capital P = multiple poses and scores and such

    // Copy the type indicator
    memcpy(msg->data(), &TYPE, sizeof(char));

    // Copy the size and timestamp
    memcpy(static_cast<void*>(static_cast<char*>(msg->data()) + 1),
	   static_cast<void*>(this),
	   sizeof(short_size_t) + sizeof(timestamp_t));

    // Figure out the start point for the matrices
    pose_output_t* p = static_cast<pose_output_t*>(static_cast<void*>(static_cast<char*>(msg->data()) + sizeof(char) + sizeof(short_size_t) + sizeof(timestamp_t)));

    // Copy the matrices
    for (short_size_t i = 0; i < size; ++i) {
      p[i] = data[i];
    }

    return msg;
  }

};


#endif // SERVICE_H
