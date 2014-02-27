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

#ifndef SCENE_H
# define SCENE_H

#include <cmath>
#include "mesh.h"

#define _USE_MATH_DEFINES

const float ASPECT_RATIO = 1.0;
const float CAMERA_Y     = 0.05;

const double RADIANS_PER_DEGREE = M_PI / 180.0;


class Scene {
public:
  Scene(const std::string& filename, float scale_factor_ = 1.0)
  : scale_factor(scale_factor_), camera_z(1000.0)
  {
    mesh.load_mesh(filename);

    glm::vec3 dimensions = mesh.dimensions();
    std::cerr << "Object dimensions as modeled: " << dimensions.x << '\t' << dimensions.y << '\t' << dimensions.z << std::endl;
  }

  void move_camera(Shader* shader_program, float z) {
    glMatrixMode(GL_PROJECTION);

    glUseProgram(shader_program->id());
    GLint camera_z_id = glGetUniformLocation(shader_program->id(), "camera_z");
    glTranslatef(0.0, 0.0, z);

    camera_z -= z;

    std::cerr << "camera_z is now " << camera_z << std::endl;
    glUniform1fv(camera_z_id, 1, &camera_z);
  }

  void rotate_model(Shader* shader_program, float x, float y, float z) {
    glMatrixMode(GL_MODELVIEW);
    glRotatef(x, 1.0, 0.0, 0.0);
    glRotatef(y, 0.0, 1.0, 0.0);
    glRotatef(z, 0.0, 1.0, 0.0);

    // Scale only the mesh.
    glScalef(scale_factor, scale_factor, scale_factor);

    // Render the mesh.
    mesh.render(shader_program);
  }


  void setup(Shader* shader_program) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glPolygonMode( GL_FRONT, GL_FILL );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    std::cerr << "Initial position is (0,0,-1000) with clipping plane 1.0, 1250.0" << std::endl;
    std::cerr << "Box is a 200 x 200 x 200 meter cube." << std::endl;
    float far_plane_ = far_plane();
    gluPerspective(20.0f, ASPECT_RATIO, 1.0, far_plane_);
    glRotatef(std::atan(CAMERA_Y/camera_z)*RADIANS_PER_DEGREE, 1, 0, 0);
    glTranslatef(0.0, CAMERA_Y, -camera_z); // move it 5cm off from the emitter.


    glUseProgram(shader_program->id());
    GLint camera_z_id = glGetUniformLocation(shader_program->id(), "camera_z");
    GLint far_plane_id = glGetUniformLocation(shader_program->id(), "far_plane");

    std::cerr << "camera_z is now " << camera_z << std::endl;
    glUniform1fv(camera_z_id, 1, &camera_z);
    glUniform1fv(far_plane_id, 1, &far_plane_);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }


  float far_plane() const {
    return camera_z * 1.25;
  }


  /*
   * Draws a simple box in green, blue, and red around the origin with dimensions 200 x 200 x 200.
   */
  void render_box() {
    glPushMatrix();

    glRotatef(45.0f, 1.0, 0.0, 0.0);
    glRotatef(45.0f, 0.0, 1.0, 0.0);

    glColor4f(0.0, 1.0, 0.0, 1.0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-100.0, -100.0, -100.0);
    glVertex3f(100.0,  -100.0, -100.0);
    glVertex3f(100.0,   100.0, -100.0);
    glVertex3f(-100.0,  100.0, -100.0);
    glEnd();

    glColor4f(1.0, 0.0, 0.0, 1.0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-100.0, -100.0, 100.0);
    glVertex3f(100.0,  -100.0, 100.0);
    glVertex3f(100.0,   100.0, 100.0);
    glVertex3f(-100.0,  100.0, 100.0);
    glEnd();

    glColor4f(0.0, 0.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex3f(-100.0, -100.0, -100.0);
    glVertex3f(-100.0, -100.0,  100.0);

    glVertex3f(100.0, -100.0, -100.0);
    glVertex3f(100.0, -100.0,  100.0);

    glVertex3f(100.0, 100.0, -100.0);
    glVertex3f(100.0, 100.0,  100.0);

    glVertex3f(-100.0, 100.0, -100.0);
    glVertex3f(-100.0, 100.0,  100.0);
    glEnd();

    glPopMatrix();
  }

  void render(Shader* shader_program, float x, float y, float z, bool box = true) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    //glBlendFuncSeparate(GL_ONE, GL_ONE, GL_SRC_ALPHA, GL_SRC_ALPHA);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDepthFunc(GL_LEQUAL);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glDepthRange(0.0, 1.0);


    // clear window with the current clearing color, and clear the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // | GL_DEPTH_BUFFER_BIT);

    // Setup model view matrix: All future transformations will affect our models (what we draw).
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // For debugging purposes, let's make sure we can see a box.
    if (box) render_box();


    glPushMatrix();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    float light_position[] = {0.0, 0.0, camera_z, 0.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 10.0);

    // Scale only the mesh.
    glScalef(scale_factor, scale_factor, scale_factor);
    glRotatef(x, 1.0, 0.0, 0.0);
    glRotatef(y, 0.0, 1.0, 0.0);
    glRotatef(z, 0.0, 0.0, 1.0);
    // Render the mesh.
    mesh.render(shader_program);

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glPopMatrix();

    // flush drawing commands
    glFlush();
  }


  /*
   * Write the current color buffer as a PCD (point cloud file).
   */
  void save_point_cloud(const std::string& filename, unsigned int width, unsigned int height) {

    std::cerr << "Saving point cloud..." << std::endl;

    // Get matrices we need for reversing the model-view-projection-clip-viewport transform.
    glm::ivec4 viewport;
    glm::dmat4 model_view_matrix, projection_matrix;
    glGetDoublev( GL_MODELVIEW_MATRIX, (double*)&model_view_matrix );
    glGetDoublev( GL_PROJECTION_MATRIX, (double*)&projection_matrix );
    glGetIntegerv( GL_VIEWPORT, (int*)&viewport );

    std::ofstream out(filename.c_str());

    // Print PCD header
    out << "VERSION .7\nFIELDS x y z intensity\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\n";
    out << "WIDTH " << width << std::endl;
    out << "HEIGHT " << height << std::endl;
    out << "VIEWPOINT 0 0 0 1 0 0 0" << std::endl;
    out << "POINTS " << width*height << std::endl;
    out << "DATA ASCII" << std::endl;

    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {

        glm::dvec3 near, far;
        glm::vec4 rgba;

        gluUnProject(i, j, 0.0, (double*)&model_view_matrix, (double*)&projection_matrix, (int*)&viewport, &(near[0]), &(near[1]), &(near[2]));
        gluUnProject(i, j, 1.0, (double*)&model_view_matrix, (double*)&projection_matrix, (int*)&viewport, &(far[0]),  &(far[1]),  &(far[2]));
        glReadPixels(i, height - j, 1, 1, GL_RGBA, GL_FLOAT, (float*)&rgba);
        glm::dvec3 relative_far = near - far;

        //std::cerr << "\tbuffer val: " << rgba[0] << '\t' << rgba[1] << '\t' << rgba[2] << '\t' << rgba[3] << std::endl;
        glm::dvec3 position = relative_far * (double)(rgba[2]);
        if (position[0] != 0) position[0] = -position[0];

        out << position[0] << ' ' << position[1] << ' ' << position[2] << ' ' << rgba[0] << '\n';
      }
    }

    out.close();

    std::cerr << "Saved '" << filename << "'" << std::endl;
  }


private:
  Mesh mesh;
  float scale_factor;
  float camera_z;
};

#endif