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
 */

#ifndef QUATERNION_H
# define QUATERNION_H

#define GLM_FORCE_RADIANS 1

#include <glm/gtc/quaternion.hpp>
#include <sstream>


std::string to_string(const glm::dquat& q) {
  std::ostringstream out;
  out << q[3] << ", " << q[0] << ", " << q[1] << ", " << q[2] << std::flush;
  return out.str();
}


/** See Markley 2003 for explanation.
 *
 * @param[in] w is the angular rate, given as an axis divided by the angle
 *
 * \returns A 4x4 matrix that can be multiplied by a quaternion to get the quaternion time derivative.
 */
glm::dmat4 big_omega(const glm::dvec3& w) {
  /* Equation 10 from Markley 2003, except we want to invert it to handle the fact that
   * Eigen stores its w before xyz instead of after. 
   * Also, we need to transpose the matrix as we initialize it. */
  glm::dmat4 o;
  double arr[16] = {  0.0,  w[0], w[1], w[2],
		     -w[0], 0.0, -w[2], w[1],
		     -w[1], w[2], 0.0, -w[0],
		     -w[2], -w[1], w[0], 0.0 };
  memcpy( glm::value_ptr(o), arr, sizeof(arr) );
  
  return o;
}


/** See Markley 2003 for explanation. Returns time derivative of some angular rate given a quaternion.
 *
 * @param[in] initial quaternion
 * @param[in] angular rate (axis divided by angle)
 *
 * \returns The time derivative as a 4-vector.
 */
glm::dvec4 quaternion_time_derivative(const glm::dquat& q, const glm::dvec3& w) {
  glm::dvec4 v(q[0], q[1], q[2], q[3]);
  v = big_omega(w)*v;
  return v;
}


/** Rotate a quaternion by some angular rate over some time. (From Markley 2003)
 *
 * @param[in] initial quaternion
 * @param[in] angular rate (axis divided by angle)
 * @param[in] delta-time
 *
 * \returns The resulting quaternion.
 */
glm::dquat quaternion_change(const glm::dquat& q, const glm::dvec3& w, double dt) {
  glm::dvec4 v = quaternion_time_derivative(q, w) * dt;
  
  glm::dquat r;
  r[0] = q[0] + v[0];
  r[1] = q[1] + v[1];
  r[2] = q[2] + v[2];
  r[3] = q[3] + v[3];

  return glm::normalize(r);
}


glm::dquat qcross(const glm::dquat& p, const glm::dquat& q) {
  glm::dvec3 qv(q[1], q[2], q[3]);
  glm::dvec3 pv(p[1], p[2], p[3]);
  glm::dvec3 rv = p[0] * qv + q[0] * pv - glm::cross(pv, qv);
  
  glm::dquat r(p[0] * q[0] - glm::dot(pv,qv),
	       rv[0], rv[1], rv[2]);
  return r;
}

#endif
