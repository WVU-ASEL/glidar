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
    Texture(GLenum texture_target_, const std::string& filename_);

    bool load();

    void bind(GLenum texture_unit, Shader& shader_program) {
      glActiveTexture(texture_unit);
      GLint texture_location = glGetUniformLocation(shader_program.id(), "texture_color");
      glUniform1i(texture_location, 0);
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

