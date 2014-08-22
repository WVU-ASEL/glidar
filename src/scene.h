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
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/projection.hpp>
#include <cmath>
#include "mesh.h"

#define _USE_MATH_DEFINES

const float ASPECT_RATIO = 1.0;
const float CAMERA_Y     = 0.05;
const unsigned int BOX_HALF_DIAGONAL = 174;
const float MIN_NEAR_PLANE = 0.01;

const double RADIANS_PER_DEGREE = M_PI / 180.0;


class Scene {
public:
  Scene(const std::string& filename, float scale_factor_, float camera_z_)
  : scale_factor(scale_factor_),
    camera_z(camera_z_),
    ideal_near_plane(camera_z-BOX_HALF_DIAGONAL),
    real_near_plane(std::max(MIN_NEAR_PLANE, ideal_near_plane)),
    far_plane(camera_z+BOX_HALF_DIAGONAL)
  {
    mesh.load_mesh(filename);

    glm::vec3 dimensions = mesh.dimensions();
    std::cerr << "Object dimensions as modeled: " << dimensions.x << '\t' << dimensions.y << '\t' << dimensions.z << std::endl;
    glm::vec3 centroid = mesh.centroid();
    std::cerr << "Center of object as modeled: " << centroid.x << '\t' << centroid.y << '\t' << centroid.z << std::endl;
    std::cerr << "NOTE: Object will be re-centered prior to rendering." << std::endl;
  }

  void move_camera(Shader* shader_program, float z) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    std::cerr << "Position is (0,0," << camera_z << ") with clipping plane " << real_near_plane << ", " << far_plane << std::endl;
    //std::cerr << "Box is a 200 x 200 x 200 meter cube." << std::endl;

    camera_z -= z;
    ideal_near_plane -= z;
    real_near_plane = std::max(MIN_NEAR_PLANE, ideal_near_plane);
    far_plane -= z;
  }


  void projection_setup(float fov) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_TEXTURE_2D); // Probably has no meaning since we're using shaders.
    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
    glDisable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    //glBlendFuncSeparate(GL_ONE, GL_ONE, GL_SRC_ALPHA, GL_SRC_ALPHA);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDepthFunc(GL_LEQUAL);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glPolygonMode( GL_FRONT, GL_FILL );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(fov, ASPECT_RATIO, real_near_plane, far_plane);
  }


  /*
   * Given some x,y coordinates in the window, and the blue channel of those coordinates, what are the world coordinates
   * of that surface?
   */
  glm::dvec3 unproject(const glm::dmat4& model_view_matrix, const glm::dmat4& projection_matrix, const glm::ivec4& viewport, size_t height, double x, double y) const {
    glm::vec4 rgba;
    return unproject(rgba, model_view_matrix, projection_matrix, viewport, height, x, y);
  }


  glm::dvec3 unproject(glm::vec4& rgba, const glm::dmat4& model_view_matrix, const glm::dmat4& projection_matrix, const glm::ivec4& viewport, size_t height, double x, double y) const {
    glReadPixels(x, height - y, 1, 1, GL_RGBA, GL_FLOAT, (float*)&rgba);

    std::cerr << "RGB12=" << rgba[1] << ',' << rgba[2] << std::endl;

    int   gb    = (rgba[1] * 65280) + (rgba[2] * 256);
    double t    = gb == 0 ? 1.0 : gb / 65536.0;
    //double dist = rgba[2] > 0 ? rgba[2] * (far_plane - real_near_plane) + real_near_plane : far_plane;
    //std::cerr << "blue channel = " << t << std::endl;
    //std::cerr << "dist = " << dist << std::endl;

    glm::dvec3 position;
    gluUnProject(x, y, t, (double*)&model_view_matrix, (double*)&projection_matrix, (int*)&viewport, &(position[0]), &(position[1]), &(position[2]) );

    std::cerr << position[0] << ',' << position[1] << ',' << position[2] << '\t';
    position = glm::dvec3(0.0, CAMERA_Y, camera_z) - position;
    std::cerr << position[0] << ',' << position[1] << ',' << position[2] << '\n';


    return position;
  }

  /*
   * See unproject above; this one is also responsible for retrieving the model view and projection matrices as well as the viewport.
   */
  glm::dvec3 unproject(size_t height, double x, double y) const {
    glm::ivec4 viewport;
    glm::dmat4 model_view_matrix, projection_matrix;

    glGetDoublev( GL_MODELVIEW_MATRIX,   (double*)&model_view_matrix);
    glGetDoublev( GL_PROJECTION_MATRIX,  (double*)&projection_matrix);
    glGetIntegerv(GL_VIEWPORT,           (int*)&viewport);

    return unproject(model_view_matrix, projection_matrix, viewport, height, x, y);
  }


  /*
   * Draws a simple box in green, blue, and red around the origin with dimensions 200 x 200 x 200.
   */
  void render_box() {
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D); // Probably has no meaning since we're using shaders.

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

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopMatrix();
  }

  void render(Shader* shader_program, float fov, float rx, float ry, float rz, bool box = true) {
    projection_setup(fov);

    // clear window with the current clearing color, and clear the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // | GL_DEPTH_BUFFER_BIT);

    // Setup model view matrix: All future transformations will affect our models (what we draw).
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glUseProgram(shader_program->id());

    float light_position[] = {0.0, 0.0, 0.0, 1.0};
    float light_direction[] = {0.0, 0.0, 1.0, 0.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 10.0);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0001f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.00000001f);


    float light_matrix[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    glGetFloatv(GL_MODELVIEW_MATRIX, light_matrix);
    GLint light_matrix_id = glGetUniformLocation(shader_program->id(), "LightModelViewMatrix");
    glUniformMatrix4fv(light_matrix_id, 1, false, light_matrix);

/*    float light_diffuse[]  = {1.0, 1.0, 1.0, 1.0};
    float light_ambient[]  = {1.0, 1.0, 1.0, 1.0};
    float light_specular[]  = {1.0, 1.0, 1.0, 1.0};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular); */


    glTranslatef(0.0, 0.0, -camera_z);
    //glRotatef(std::atan(-CAMERA_Y/camera_z)*RADIANS_PER_DEGREE, 1, 0, 0); // This would be used to move the camera slightly away from the laser source

    // For debugging purposes, let's make sure we can see a box.
    glUseProgramObjectARB(0);
    if (box) render_box();
    glUseProgram(shader_program->id());

    glPushMatrix();



    GLint camera_z_id = glGetUniformLocation(shader_program->id(), "camera_z");
    GLint far_plane_id = glGetUniformLocation(shader_program->id(), "far_plane");
    GLint near_plane_id = glGetUniformLocation(shader_program->id(), "near_plane");

    glUniform1fv(camera_z_id, 1, &camera_z);
    glUniform1fv(far_plane_id, 1, &far_plane);
    glUniform1fv(near_plane_id, 1, &real_near_plane);

    check_gl_error();


    // Translate and scale only the mesh.
    glm::vec3 centroid(mesh.centroid());

    glRotatef(rz, 0.0, 0.0, 1.0);
    glRotatef(ry, 0.0, 1.0, 0.0);
    glRotatef(rx, 1.0, 0.0, 0.0);

    glScalef(scale_factor, scale_factor, scale_factor);
    //glTranslatef(-centroid.x, -centroid.y, -centroid.z);

    check_gl_error();

    // Render the mesh.
    mesh.render(shader_program);

    check_gl_error();

    glPopMatrix();

    // flush drawing commands
    glFlush();
  }


  /*
   * Write the translation and rotation information to a file.
   */
  void save_transformation_metadata(const std::string& basename, float rx, float ry, float rz) {
    std::string filename = basename + ".transform";
    std::ofstream out(filename);

    out << get_camera_z() << '\n';
    out << rx << '\t' << ry << '\t' << rz << std::endl;

    out.close();
  }


  /*
   * Write the current color buffer as a PCD (point cloud file) (ASCII version).
   */
  void save_point_cloud_ascii(const std::string& basename, unsigned int width, unsigned int height) {
    std::string filename = basename + ".pcd";

    std::cerr << "Saving point cloud..." << std::endl;

    // Get matrices we need for reversing the model-view-projection-clip-viewport transform.
    glm::ivec4 viewport;
    glm::dmat4 model_view_matrix, projection_matrix;
    glGetDoublev( GL_MODELVIEW_MATRIX, (double*)&model_view_matrix );
    glGetDoublev( GL_PROJECTION_MATRIX, (double*)&projection_matrix );
    glGetIntegerv( GL_VIEWPORT, (int*)&viewport );

    std::ofstream out(filename);

    // Print PCD header
    out << "VERSION .7\nFIELDS x y z intensity\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\n";
    out << "WIDTH " << width << std::endl;
    out << "HEIGHT " << height << std::endl;
    out << "VIEWPOINT 0 0 0 1 0 0 0" << std::endl;
    out << "POINTS " << width*height << std::endl;
    out << "DATA ascii" << std::endl;

    // If I had a newer graphics card, this could probably be done in-GPU instead of in this loop, which really takes
    // forever to run.
    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {

        // unproject writes into rgba
        glm::vec4 rgba;
        glm::dvec3 position = unproject(rgba, model_view_matrix, projection_matrix, viewport, height, i, j);
        //std::cerr << "\tbuffer val: " << rgba[0] << '\t' << rgba[1] << '\t' << rgba[2] << '\t' << rgba[3] << std::endl;

        out << position[0] << ' ' << position[1] << ' ' << position[2] << ' ' << rgba[0] << '\n';
      }
    }

    out.close();

    std::cerr << "Saved '" << filename << "'" << std::endl;
  }

  /*
  * Write the current color buffer as a PCD (point cloud file) (binary version).
  */
  void save_point_cloud_organized(const std::string& basename, unsigned int width, unsigned int height) {
    std::string filename = basename + ".pcd";

    std::cerr << "Saving point cloud..." << std::endl;

    // Get matrices we need for reversing the model-view-projection-clip-viewport transform.
    glm::ivec4 viewport;
    glm::dmat4 model_view_matrix, projection_matrix;
    glGetDoublev( GL_MODELVIEW_MATRIX, (double*)&model_view_matrix );
    glGetDoublev( GL_PROJECTION_MATRIX, (double*)&projection_matrix );
    glGetIntegerv( GL_VIEWPORT, (int*)&viewport );

    std::ofstream out(filename);

    // Print PCD header
    out << "VERSION .7\nFIELDS x y z intensity\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\n";
    out << "WIDTH " << width << std::endl;
    out << "HEIGHT " << height << std::endl;
    out << "VIEWPOINT 0 0 0 1 0 0 0" << std::endl;
    out << "POINTS " << width*height << std::endl;
    out << "DATA binary" << std::endl;

    // If I had a newer graphics card, this could probably be done in-GPU instead of in this loop, which really takes
    // forever to run.
    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {

        // unproject writes into rgba
        glm::vec4 rgba;
        glm::dvec3 position = unproject(rgba, model_view_matrix, projection_matrix, viewport, height, i, j);
        //std::cerr << "\tbuffer val: " << rgba[0] << '\t' << rgba[1] << '\t' << rgba[2] << '\t' << rgba[3] << std::endl;

        float data[4];
        data[0] =  (float)position[0];
        data[1] =  (float)position[1];
        data[2] =  (float)position[2];
        data[3] =  (float)rgba[0];
        out.write((char*)(data), sizeof(float)*4);
      }
    }

    out.close();

    std::cerr << "Saved '" << filename << "'" << std::endl;
  }

  /*
  * Write the current color buffer as a PCD (point cloud file) (binary non-organized version).
  */
  void save_point_cloud(const std::string& basename, unsigned int width, unsigned int height) {
    std::string filename = basename + ".pcd";

    std::cerr << "Saving point cloud..." << std::endl;

    // Get matrices we need for reversing the model-view-projection-clip-viewport transform.
    glm::ivec4 viewport;
    glm::dmat4 model_view_matrix, projection_matrix;
    glGetDoublev( GL_MODELVIEW_MATRIX, (double*)&model_view_matrix );
    glGetDoublev( GL_PROJECTION_MATRIX, (double*)&projection_matrix );
    glGetIntegerv( GL_VIEWPORT, (int*)&viewport );

    float* data   = new float[4*width*height];
    size_t data_count = 0;
    // If I had a newer graphics card, this could probably be done in-GPU instead of in this loop, which really takes
    // forever to run.
    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {

        // unproject writes into rgba
        glm::vec4 rgba;

        glReadPixels(i, height - j, 1, 1, GL_RGBA, GL_FLOAT, (float*)&rgba);
        int gb = (rgba[1] * 65280.0) + (rgba[2] * 256.0);
        if (gb == 0) continue;
        double t = gb / 65536.0;

        // Just out of curiosity, what happens if we calculate d ourselves?
        double d_green = rgba[1] * 65280.0;
        double d_blue  = rgba[2] * 256.0;
        double d = ((d_green + d_blue) / 65536.0) * (far_plane - real_near_plane) + real_near_plane;


        glm::dvec3 position;
        gluUnProject(i, j, t, (double*)&model_view_matrix, (double*)&projection_matrix, (int*)&viewport, &(position[0]), &(position[1]), &(position[2]) );

        position = glm::dvec3(0.0, CAMERA_Y, camera_z) - position;

        data[data_count]   =  (float)position[0];
        data[data_count+1] =  (float)position[1];
        data[data_count+2] =  (float)d; //(float)position[2];
        data[data_count+3] =  (float)rgba[0];

        //std::cerr << "position[2]=" << position[2] << " versus " << d_green << " and " << d_blue << " ( g = " << rgba[1] << ", b = " << rgba[2] << " ) so " << d << std::endl;

        data_count += 4;
      }
    }
    //std::cerr << "far plane = " << far_plane << "\t near plane = " << real_near_plane << std::endl;

    std::ofstream out(filename);
    // Print PCD header
    out << "VERSION .7\nFIELDS x y z intensity\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\n";
    out << "WIDTH " << data_count / 4 << std::endl;
    out << "HEIGHT " << 1 << std::endl;
    out << "VIEWPOINT 0 0 0 1 0 0 0" << std::endl;
    out << "POINTS " << data_count / 4 << std::endl;
    out << "DATA binary" << std::endl;
    out.write((char*)(data), sizeof(float)*data_count);

    out.close();

    std::cerr << "Saved '" << filename << "'" << std::endl;
  }

  float get_camera_z() const { return camera_z; }
  float get_near_plane() const { return real_near_plane; }
  float get_far_plane() const { return far_plane; }
private:
  Mesh mesh;
  float scale_factor;
  GLfloat camera_z;
  GLfloat ideal_near_plane;
  GLfloat real_near_plane;
  GLfloat far_plane;
};

#endif