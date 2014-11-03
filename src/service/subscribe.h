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

#ifndef SUBSCRIBE_H
# define SUBSCRIBE_H

#include "service.h"

#include <zmq.hpp>
#include <pcl/point_cloud.h>

#include <cstdio> // strncmp

enum recv_result_t {
  RECV_SHUTDOWN,
  RECV_SUCCESS,
  RECV_NOUPDATE,
  RECV_FAILURE
};

/** \brief Open a subscription socket which makes the publisher wait
  * for synchronization before sending.
  * \param[in] socket through which to subscribe
  * \param[in] socket through which to synchronize
  * \param[in] starting TCP port (subscription port; port+1 will be used for
  * synchronization TCP port)
  * \param[in] highwater mark for receiving (drop after this many in queue)
  * \param[in] type of messages to receive (p for pose, c for cloud)
  */
void sync_subscribe(zmq::socket_t& subscriber, zmq::socket_t& sync_client, int port, int highwater, char type);


/* \brief Open a subscription socket which doesn't make the publisher
 * wait for synchronization.
 * \param[in] socket through which to subscribe
 * \param[in] socket through which to synchronize
 * \param[in] starting TCP port (subscription port; port+1 will be used for
 * synchronization TCP port)
 * \param[in] highwater mark for receiving (drop after this many in queue)
 */
void subscribe(zmq::socket_t& subscriber, int port, int highwater);


bool received_shutdown(zmq::message_t& msg);


/** \brief receives a pose from a publisher
 *  \param[in] subscriber the socket through which the data will be received
 *  \param[out] pose the 4x4 matrix that is received
 *  \param[out] timestamp for which the pose was computed
 *  \param[in] flags to pass to zmq recv (mainly only 0 or ZMQ_NOBLOCK)
 *  \return recv_result_t indicating whether a new point cloud was received or a shutdown signal
 */ 
recv_result_t receive_pose(zmq::socket_t& subscriber, Eigen::Matrix4f& pose, timestamp_t& timestamp, int flags = 0);


/** \brief receives a set of poses and scores from a publisher
 *  \param[in] subscriber the socket through which the data will be received
 *  \param[out] poses the pose messages received along with the timestamp
 *  \param[in] flags to pass to zmq recv (mainly only 0 or ZMQ_NOBLOCK)
 *  \return recv_result_t indicating whether a new point cloud was received or a shutdown signal
 */ 
recv_result_t receive_poses(zmq::socket_t& subscriber, pose_message_t::ptr& poses, int flags = 0);


/** \brief receives a point cloud from a publisher
 *  \param[in] subscriber the socket through which the data will be received
 *  \param[out] cloud the point cloud that is received
 *  \param[out] timestamp at which the point cloud was generated
 *  \param[in] flags to pass to zmq recv (mainly only 0 or ZMQ_NOBLOCK)
 *  \return recv_result_t indicating whether a new point cloud was received or a shutdown signal
 */ 
template <typename PointT>
recv_result_t receive_disorganized_point_cloud(zmq::socket_t& subscriber, typename pcl::PointCloud<PointT>::Ptr& cloud, timestamp_t& timestamp, int flags = 0) {
  
  zmq::message_t message;
  bool result = subscriber.recv(&message, flags);

  // If ZMQ_NOBLOCK is passed in as a flag, we don't actually need to receive any result
  // to report success. The only time we report failure is if a shutdown message is
  // received.
  if ((flags & ZMQ_NOBLOCK) && !result)
    return RECV_NOUPDATE;
  else if (!result)
    return RECV_FAILURE;

  float* data = NULL;

  if (received_shutdown(message)) {
    return RECV_SHUTDOWN;

  } else {
    size_t size = (message.size() - sizeof(timestamp_t) - sizeof(char)) / (4*sizeof(float));
    void* timestamp_pos = static_cast<void*>(static_cast<char*>(message.data()) + 1);

    // Get the timestamp
    memcpy(&timestamp, timestamp_pos, sizeof(timestamp_t));

    data = static_cast<float*>(static_cast<void*>(static_cast<char*>(message.data()) + sizeof(timestamp_t) + sizeof(char)));

    std::cerr << "Received a point cloud of size " << size << " points" << std::endl;
    // allocate the cloud with the correct size right off the bat.
    cloud.reset(new pcl::PointCloud<PointT>(size, 1));

    for (size_t i = 0; i < size; ++i) {
      cloud->points[i].x = data[i*4 + 0];
      cloud->points[i].y = data[i*4 + 1];
      cloud->points[i].z = data[i*4 + 2];
      //cloud.points[i].intensity = data[i*4 + 3];     
    }
    //memcpy(&(cloud->points), static_cast<float*>(message.data()), message.size());
    
    return RECV_SUCCESS;
  }
}


/** \brief receives a point cloud from a publisher (dropping earlier messages)
 *  \param[in] subscriber the socket through which the data will be received
 *  \param[out] cloud the point cloud that is received
 *  \return boolean indicating whether we can continue or whether we need to terminate
 */ 
template <typename PointT>
bool receive_most_recent_disorganized_point_cloud(zmq::socket_t& subscriber, typename pcl::PointCloud<PointT>::Ptr& cloud, timestamp_t& timestamp) {
  
  zmq::message_t message;
  int events = 0;
  size_t events_size = sizeof(int);
  subscriber.recv(&message);
  subscriber.getsockopt(ZMQ_EVENTS, static_cast<void*>(&events), &events_size);
  while (events & ZMQ_POLLIN) {
    subscriber.recv(&message);
    subscriber.getsockopt(ZMQ_EVENTS, static_cast<void*>(&events), &events_size);
  }
  float* data = NULL;

  if (received_shutdown(message)) {
    return false;

  } else {
    size_t size = (message.size() - sizeof(timestamp_t) - sizeof(char)) / (4*sizeof(float));
    void* timestamp_pos = static_cast<void*>(static_cast<char*>(message.data()) + 1);

    // Get the timestamp
    memcpy(&timestamp, timestamp_pos, sizeof(timestamp_t));

    data = static_cast<float*>(static_cast<void*>(static_cast<char*>(message.data()) + sizeof(timestamp_t) + sizeof(char)));

    std::cerr << "Received a point cloud of size " << size << " points" << std::endl;
    // allocate the cloud with the correct size right off the bat.
    cloud.reset(new pcl::PointCloud<PointT>(size, 1));

    for (size_t i = 0; i < size; ++i) {
      cloud->points[i].x = data[i*4 + 0];
      cloud->points[i].y = data[i*4 + 1];
      cloud->points[i].z = data[i*4 + 2];
      //cloud.points[i].intensity = data[i*4 + 3];     
    }
    //memcpy(&(cloud->points), static_cast<float*>(message.data()), message.size());
    
    return true;

  }
}


/** \brief receives a vector of some type
 *  \param[in] subscriber
 *  \param[out] values the values that will be published
 *  \param[in] flags for blocking and such
 */
template <typename T>
recv_result_t receive_vector(zmq::socket_t& subscriber, timestamp_t& timestamp, typename std::vector<T>& values, int flags = 0) {
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

  } else {
    size_t size = (message.size() - sizeof(char) - sizeof(timestamp_t)) / (sizeof(T));
    timestamp_t* p_timestamp = reinterpret_cast<timestamp_t*>(static_cast<char*>(message.data()) + 1);
    timestamp = *p_timestamp;

    T* p_data = reinterpret_cast<T*>(&(p_timestamp[1]));
    values.resize(size);
    for (size_t i = 0; i < size; ++i) {
      values[i] = p_data[i];
    }

    return RECV_SUCCESS;
  }
}


#endif // SUBSCRIBE_H
