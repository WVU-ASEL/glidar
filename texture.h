/*

	Copyright 2011 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TEXTURE_H
#define	TEXTURE_H

#include <string>

#include <GL/glew.h>
#include <Magick++.h>

#include "gl_error.h"
#include "shader.h"

class Texture
{
public:
  Texture(const std::vector<std::string>& filenames_)
  : filenames(filenames_),
    texture_object_handles(new GLuint[filenames_.size()]),
    images(filenames_.size(), NULL),
    blobs(filenames_.size())
  { }

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

