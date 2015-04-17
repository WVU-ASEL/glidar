/*
 * Copyright (c) 2014-2015, John O. Woods, Ph.D.
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

#ifndef POSE_PCL_H
# define POSE_PCL_H

#include <pcl/console/parse.h>

#include "quaternion.h"

namespace pcl { namespace console {
  // Read as if it is an angle axis: theta,x,y,z 
  bool parse_4x_arguments(int argc, char** argv, const char* str, glm::dquat& v) {
    std::vector<double> tmp;
    double angle;
    glm::dvec3 xyz(1.0, 0.0, 0.0);
    pcl::console::parse_x_arguments(argc, argv, str, tmp);

    if (tmp.size() == 0) {
      return false;
    } else if (tmp.size() == 1) { // use about unit x
      angle = 0.0;
    } else if (tmp.size() == 4) {
      angle = tmp[0];
      xyz[0] = tmp[1];
      xyz[1] = tmp[2];
      xyz[2] = tmp[3];
    } else {
      std::cerr << "invalid angle axis (wrong number of arguments)" << std::endl;
      return false;
    }

    v = glm::angleAxis<double>(angle, xyz);
    return true;
  }

  bool parse_3x_arguments(int argc, char** argv, const char* str, glm::dvec3& v) {
    return parse_3x_arguments(argc, argv, str, v[0], v[1], v[2]);
  }

  bool parse_3x_arguments(int argc, char** argv, const char* str, glm::vec3& v) {
    return parse_3x_arguments(argc, argv, str, v[0], v[1], v[2]);
  }
}}


#endif // POSE_PCL_H
