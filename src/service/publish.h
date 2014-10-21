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

#ifndef PUBLISH_H
# define PUBLISH_H

#include "service.h"

#include <pcl/point_cloud.h>

void cpp_message_free(float* data, void* hint);

extern "C" {
  void message_free(void* data, void* hint);
  void c_message_free(void* data, void* hint);
}


void send_shutdown(zmq::socket_t& publisher);


/* \brief Open a publish socket which makes the publisher wait for
 * synchronization before sending.
 * \param socket through which to publish
 * \param socket through which to synchronize
 * \param starting TCP port (publishing port; port+1 will be used for
 * synchronization TCP port)
 * \param number of subscribers to wait for (more are allowed, but
 * this many will be required before publishing begins)
 */
void sync_publish(zmq::socket_t& publisher, zmq::socket_t& sync_service, int port, size_t expected_subscribers = 1);


/** \brief sends a pose to subscribers
  * \param[in] publisher the socket through which the data will be sent
  * \param[out] timestamp the timestamp of the sensor image to which the pose corresponds
  * \param[out] pose the 4x4 matrix that is to be sent
  */ 
void send_pose(zmq::socket_t& publisher, const Eigen::Matrix4f& pose, const timestamp_t& timestamp);


/** \brief sends a set of poses and scores to subscribers
  * \param[in] publisher the socket through which the data will be sent
  * \param[out] pose the 4x4 matrix that is to be sent
  */ 
void send_poses(zmq::socket_t& publisher, const pose_message_t::ptr& poses);


/** \brief sends a disorganized point cloud to subscribers (assumption 
  * is that both the sender and recipient have the same size PointT)
  * \param[in] publisher the socket through which the data will be sent
  * \param[out] pose the 4x4 matrix that is to be sent
  */ 
template <typename PointT>
void send_disorganized_point_cloud(zmq::socket_t& publisher, const timestamp_t& timestamp, typename pcl::PointCloud<PointT>::ConstPtr cloud) {

  size_t cloud_size = cloud->width * cloud->height * sizeof(PointT);
  void* send_buffer = malloc(sizeof(timestamp_t) + cloud_size + 1);
  void* timestamp_buffer = static_cast<void*>(static_cast<char*>(send_buffer) + 1);
  void* cloud_buffer = static_cast<void*>(static_cast<char*>(send_buffer) + sizeof(timestamp_t) + 1);

  const char TYPE = 'c';

  memcpy(send_buffer, &TYPE, sizeof(char));
  memcpy(timestamp_buffer, &timestamp, sizeof(timestamp_t));
  memcpy(cloud_buffer, &(cloud->points), cloud_size);

  zmq::message_t message(send_buffer, sizeof(timestamp_t) + sizeof(char) + cloud_size, message_free, NULL);
  publisher.send(message);
}

#endif // PUBLISH_H
