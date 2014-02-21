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

#include "shader.h"

class Texture
{
public:
    Texture(GLenum texture_target_, const std::string& filename_)
    : filename(filename_),
      texture_target(texture_target_),
      image(NULL)
    { }

    bool load() {
      try {
        std::cerr << "Attempting to load texture from " << filename << std::endl;
        image = new Magick::Image(filename);
        image->write(&blob, "RGBA");
      }
      catch (Magick::Error& error) {
        std::cout << "Error loading texture '" << filename << "': " << error.what() << std::endl;
        return false;
      }

      glGenTextures(1, &texture_object);
      glBindTexture(texture_target, texture_object);
      glTexImage2D(texture_target, 0, GL_RGB, image->columns(), image->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blob.data());
      glTexParameterf(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      return true;
    }


  void bind(GLenum texture_unit, Shader* shader_program = NULL) {
      glActiveTexture(texture_unit);

      if (shader_program) {
        GLint texture_location = glGetUniformLocation(shader_program->id(), "texture_color");
        glUniform1i(texture_location, 0);
      }

      glBindTexture(texture_target, texture_object);
    }

private:
    std::string filename;
    GLenum texture_target;
    GLuint texture_object;
    Magick::Image* image;
    Magick::Blob    blob;
};


#endif	/* TEXTURE_H */

