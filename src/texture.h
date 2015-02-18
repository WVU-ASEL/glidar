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

// This file based loosely on a GPLv3 example given by Etay Meiri. I hope I've
// changed it sufficiently that it's no longer a derivative work, but please feel
// free to contact me if you feel I've violated the spirit of the GPL.

#ifndef TEXTURE_H
#define	TEXTURE_H

#include <string>

#include <GL/glew.h>
#include <Magick++.h>

#include "gl_error.h"
#include "shader.h"

/** For loading and rendering textures. 
 * 
 * Each texture represents a specific aspect of the object's lighting/surface model (analogous to a 
 * bidirectional reflectance function), or helps define its normal maps.
 */
class Texture {
public:
  /** Constructor for textures.
   *
   * @param[in] vector of filenames where the textures may be found.
   */
  Texture(const std::vector<std::string>& filenames_)
  : filenames(filenames_),
    texture_object_handles(new GLuint[filenames_.size()]),
    images(filenames_.size(), NULL),
    blobs(filenames_.size())
  { }

  /** Load textures and register them with OpenGL.
   *
   */
  bool load() {
    for (size_t i = 0; i < filenames.size(); ++i) {
      try {
        std::cerr << "Attempting to load texture from " << filenames[i] << std::endl;
        images[i] = new Magick::Image(filenames[i]);
        images[i]->write(&(blobs[i]), "RGBA");
      }
      catch (Magick::Error &error) {
        std::cout << "Error loading texture '" << filenames[i] << "': " << error.what() << std::endl;
        return false;
      }
    }

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(filenames.size(), texture_object_handles);

    for (size_t i = 0; i < filenames.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i); // next few calls apply to this texture
      glBindTexture(GL_TEXTURE_2D, texture_object_handles[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, images[i]->columns(), images[i]->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blobs[i].data());
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    check_gl_error();

    return true;
  }


  /** Bind a GLSL shader program for rendering.
   *
   * @param[in] Pointer to a shader program that we want to bind.
   */
  void bind(Shader* shader_program = NULL) {
    const char* UNIFORM_NAMES[] = {"diffuse_texture_color",
        "specular_texture_color"};

    if (shader_program) {
      for (size_t i = 0; i < filenames.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        check_gl_error();
        GLint location = glGetUniformLocation(shader_program->id(), UNIFORM_NAMES[i]);
        check_gl_error();
        glUniform1i(location, i);
        check_gl_error();

        glBindTexture(GL_TEXTURE_2D, texture_object_handles[i]);
      }
    } else { // not 100% sure this will work, but it probably doesn't matter.
      for (size_t i = 0; i < filenames.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texture_object_handles[i]);
      }
    }
  }

private:
  std::vector<std::string> filenames;
  GLuint* texture_object_handles;
  std::vector<Magick::Image*> images;
  std::vector<Magick::Blob>   blobs;
};


#endif	/* TEXTURE_H */

