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

#include <iostream>
#include <limits>
#include <Eigen/Dense>
#include <Eigen/StdVector>

struct pose_output_t {
  typedef float score_t;

  float pose[16];
  bool converged;
  score_t score;

  pose_output_t()
  : converged(false),
    score(std::numeric_limits<score_t>::infinity())
  {
    // Set identity matrix
    pose[0] = pose[5] = pose[10] = pose[15] = 1.0;
    pose[1] = pose[2] = pose[3] = pose[4] = pose[6] = pose[7] = pose[8] = pose[9] = pose[11] = pose[12] = pose[13] = pose[14] = 0.0;
  }

  pose_output_t& operator=(const pose_output_t& rhs) {
    converged = rhs.converged;
    score = rhs.score;

    for (size_t i = 0; i < 16; ++i)
      pose[i] = rhs.pose[i];

    return *this;
  }

  void get_pose(Eigen::Matrix4f& p) const {
    memcpy(p.data(), static_cast<const void*>(pose), sizeof(float)*16);
  }
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

  pose_message_t(timestamp_t t, const transform_vector_t& transforms, std::vector<float> scores)
  : timestamp(t),
    size((short_size_t)(transforms.size())),
    data(new pose_output_t[size])
  {
    for (short_size_t i = 0; i < size; ++i) {
      memcpy(static_cast<void*>(data[i].pose), transforms[i].data(), sizeof(float)*16);
      data[i].score = scores[i];
    }
  }

  pose_message_t(timestamp_t t, const Eigen::Matrix4f& pose, float score = 0.0)
  : timestamp(t),
    size(1),
    data(new pose_output_t[1])
  {
    memcpy(static_cast<void*>(data[0].pose), pose.data(), sizeof(float)*16);
    data[0].score = score;
  }

  const pose_output_t& operator[](short_size_t k) const {
    return data[k];
  }

  pose_output_t& operator[](short_size_t k) {
    return data[k];
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


/*
 * Send a message formatted as follows: prefix, timestamp, data item size (a check), number of items, items.
 */
template <typename T>
boost::shared_ptr<zmq::message_t> to_zmq(char prefix, const timestamp_t& timestamp, const typename std::vector<T>& data) {
  boost::shared_ptr<zmq::message_t> msg(new zmq::message_t(sizeof(char) + 2*sizeof(size_t) + data.size()*sizeof(T)));
  char* prefix_ptr = static_cast<char*>(msg->data());
  timestamp_t* timestamp_ptr = reinterpret_cast<timestamp_t*>(prefix_ptr + 1);
  size_t* datum_size_ptr = reinterpret_cast<size_t*>(prefix_ptr + 1 + sizeof(timestamp_t));
  size_t* data_count_ptr = reinterpret_cast<size_t*>(prefix_ptr + 1 + sizeof(timestamp_t) + sizeof(size_t));
  T* data_ptr = reinterpret_cast<T*>(prefix_ptr + 1 + sizeof(timestamp_t) + sizeof(size_t)*2);
  
  *prefix_ptr = prefix;
  *timestamp_ptr = timestamp;
  *datum_size_ptr = sizeof(T);
  *data_count_ptr = data.size();

  for (size_t i = 0; i < data.size(); ++i) {
    data_ptr[i] = data[i];
  }

  return msg;
}

/*
 * Receive a message formatted as follows: prefix, timestamp, data item size, number of item, items.
 */
template <typename T>
bool from_zmq(const zmq::message_t& msg, char& prefix, timestamp_t& timestamp, typename std::vector<T>& data) {
  const char* prefix_ptr = static_cast<const char*>(msg.data());
  const timestamp_t* timestamp_ptr = reinterpret_cast<const timestamp_t*>(prefix_ptr + 1);
  const size_t* datum_size_ptr = reinterpret_cast<const size_t*>(prefix_ptr + 1 + sizeof(timestamp_t));
  const size_t* data_count_ptr = reinterpret_cast<const size_t*>(prefix_ptr + 1 + sizeof(timestamp_t) + sizeof(size_t));
  const T* data_ptr = reinterpret_cast<const T*>(prefix_ptr + 1 + sizeof(timestamp_t) + sizeof(size_t)*2);

  prefix = *prefix_ptr;
  timestamp = *timestamp_ptr;
  size_t datum_size = *datum_size_ptr, data_count = *data_count_ptr;

  if (datum_size != sizeof(T)) {
    std::cerr << "Error: wrong from_zmq used for message; sizes don't match." << std::endl;
    return false;
  }

  data.resize(data_count);
  for (size_t i = 0; i < data_count; ++i) {
    data[i] = data_ptr[i];
  }

  return true;
}


#endif // SERVICE_H
