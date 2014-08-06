#ifndef GL_ERROR_H

#include <iostream>
#include <GL/glew.h>

// from: http://blog.nobel-joergensen.com/2013/01/29/debugging-opengl-using-glgeterror/
void _check_gl_error(const char *file, int line);

#define check_gl_error() _check_gl_error(__FILE__,__LINE__)

#endif