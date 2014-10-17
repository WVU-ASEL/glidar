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

#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/string_cast.hpp>
#include <cmath>
#include "mesh.h"

#define _USE_MATH_DEFINES

const float ASPECT_RATIO = 1.0;
const float CAMERA_Y     = 0.05;
const unsigned int BOX_HALF_DIAGONAL = 174;
const GLfloat MIN_NEAR_PLANE = 0.01;

const double RADIANS_PER_DEGREE = M_PI / 180.0;


class Scene {
public:
  Scene(const std::string& filename, float scale_factor_, float camera_z_)
  : scale_factor(scale_factor_),
    projection(1.0),
    camera_pos(0.0, 0.0, camera_z_, 0.0),
    camera_dir(0.0, 0.0, -1.0, 0.0),
    near_plane_bound(camera_z_ - BOX_HALF_DIAGONAL),
    real_near_plane(std::max(MIN_NEAR_PLANE, camera_z_-BOX_HALF_DIAGONAL)),
    far_plane(camera_z_+BOX_HALF_DIAGONAL)
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

    std::cerr << "Position is (" << camera_pos.x << "," << camera_pos.y << "," << camera_pos.z << ") with clipping plane " << real_near_plane << ", " << far_plane << std::endl;

    // If the change in camera position is too great, reduce that change.
    while (z >= real_near_plane)
      z /= 2.0;

    camera_pos.z -= z;
    near_plane_bound -= z;
    real_near_plane = std::max(MIN_NEAR_PLANE, near_plane_bound * 0.99f);
    far_plane -= z;
  }


  void projection_setup(float fov, float rx, float ry, float rz) {
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


    // Figure out where the near plane belongs.
    //glGetFloatv(GL_MODELVIEW_MATRIX, static_cast<GLfloat*>(glm::value_ptr(model_view)));
    glm::mat4 model_view = get_model_view_matrix(rx, ry, rz);

    // Get the camera position in object coordinates so we can find the near plane.
    glm::mat4 inverse_model_view = glm::inverse(model_view);
    glm::vec4 camera_pos_oc = inverse_model_view * camera_pos;
    glm::vec4 camera_dir_oc = inverse_model_view * camera_dir;

    glm::vec4 nearest_point_oc, nearest_point;
    mesh.nearest_point(camera_pos_oc, nearest_point_oc);
    nearest_point = model_view * nearest_point_oc;
    
    near_plane_bound = mesh.near_plane_bound(model_view, camera_pos_oc);
    real_near_plane = near_plane_bound*0.99;


    gluPerspective(fov, ASPECT_RATIO, real_near_plane, far_plane);

    // Store a copy of the current projection matrix.
    glGetDoublev( GL_PROJECTION_MATRIX, glm::value_ptr(projection));
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
    position = glm::dvec3(camera_pos) - position;
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
  void render_box(float r = 100.0) {
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D); // Probably has no meaning since we're using shaders.

    glRotatef(45.0f, 1.0, 0.0, 0.0);
    glRotatef(45.0f, 0.0, 1.0, 0.0);

    glColor4f(0.0, 1.0, 0.0, 1.0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-r, -r, -r);
    glVertex3f(r,  -r, -r);
    glVertex3f(r,   r, -r);
    glVertex3f(-r,  r, -r);
    glEnd();

    glColor4f(1.0, 0.0, 0.0, 1.0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-r, -r, r);
    glVertex3f( r, -r, r);
    glVertex3f( r,  r, r);
    glVertex3f(-r,  r, r);
    glEnd();

    glColor4f(0.0, 0.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex3f(-r, -r, -r);
    glVertex3f(-r, -r,  r);

    glVertex3f(r, -r, -r);
    glVertex3f(r, -r,  r);

    glVertex3f(r,  r, -r);
    glVertex3f(r,  r,  r);

    glVertex3f(-r, r, -r);
    glVertex3f(-r, r,  r);
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopMatrix();
  }

  void render(Shader* shader_program, float fov, float rx, float ry, float rz, bool box = true) {
    projection_setup(fov, rx, ry, rz);

    // clear window with the current clearing color, and clear the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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


    // Right now, model view is just an identity matrix, and we'll use it to place the light.
    glm::mat4 model_view;
    glGetFloatv(GL_MODELVIEW_MATRIX, static_cast<GLfloat*>(glm::value_ptr(model_view)));

    glm::mat4 light_matrix = model_view;
    GLint light_matrix_id = glGetUniformLocation(shader_program->id(), "LightModelViewMatrix");
    glUniformMatrix4fv(light_matrix_id, 1, false, static_cast<GLfloat*>(glm::value_ptr(light_matrix)));

    glTranslatef(-camera_pos.x, -camera_pos.y, -camera_pos.z);

    // For debugging purposes, let's make sure we can see a box.
    glUseProgramObjectARB(0);
    if (box) render_box(100.0);
    glUseProgram(shader_program->id());

    glPushMatrix();

    GLint far_plane_id = glGetUniformLocation(shader_program->id(), "far_plane");
    GLint near_plane_id = glGetUniformLocation(shader_program->id(), "near_plane");

    glUniform1fv(far_plane_id, 1, &far_plane);
    glUniform1fv(near_plane_id, 1, &real_near_plane);

    check_gl_error();

    // Rotate and scale only the mesh.
    glRotatef(rz, 0.0, 0.0, 1.0);
    glRotatef(ry, 0.0, 1.0, 0.0);
    glRotatef(rx, 1.0, 0.0, 0.0);

    glScalef(scale_factor, scale_factor, scale_factor);

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
    std::ofstream out(filename.c_str());

    out << get_camera_z() << '\n';
    out << rx << '\t' << ry << '\t' << rz << std::endl;

    out.close();
  }


  /*
   * Return the transformation metadata as a 4x4 homogeneous matrix.
   */
  Eigen::Matrix4f get_pose(float rx, float ry, float rz) {
    using Eigen::Vector3f;
    using Eigen::Affine;
    using Eigen::Transform;
    using Eigen::Translation3f;
    using Eigen::AngleAxisf;
    using Eigen::Matrix4f;
    using Eigen::Scaling;
   
    AngleAxisf model_rx(rx * M_PI / 180.0, Vector3f::UnitX());
    AngleAxisf model_ry(ry * M_PI / 180.0, Vector3f::UnitY());
    AngleAxisf model_rz(rz * M_PI / 180.0, Vector3f::UnitZ());

    Translation3f model_to_camera_translate(Vector3f(-camera_pos.x, -camera_pos.y, -camera_pos.z));
    AngleAxisf    model_to_camera_rotate(M_PI, Vector3f::UnitY());
  
    Eigen::Transform<float,3,Affine> result;
    result =  model_to_camera_rotate * model_to_camera_translate *
              model_rz * model_ry * model_rx;
  
    return result.matrix();
  }

  /*
   * Get the model view matrix before the scene is rendered.
   */
  glm::mat4 get_model_view_matrix(float rx, float ry, float rz) {
    return //glm::rotate(glm::mat4(1.0f), (float)(M_PI), glm::vec3(0.0, 1.0, 0.0)) *
      glm::translate(glm::mat4(1.0f), -glm::vec3(camera_pos)) *
      glm::rotate(glm::mat4(1.0f), rz, glm::vec3(0.0, 0.0, 1.0)) *
      glm::rotate(glm::mat4(1.0f), ry, glm::vec3(0.0, 1.0, 0.0)) *
      glm::rotate(glm::mat4(1.0f), rx, glm::vec3(1.0, 0.0, 0.0)) *
      glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor, scale_factor, scale_factor));
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

    std::ofstream out(filename.c_str());

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

    std::ofstream out(filename.c_str());

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
   * Writes the point cloud to a buffer as x,y,z,i. Returns a size_t
   * indicating the number of floating point entries written (note:
   * not the number of bytes written).
   */
  size_t write_point_cloud(float* data, unsigned int width, unsigned int height) {
    // Get matrices we need for reversing the model-view-projection-clip-viewport transform.
    glm::ivec4 viewport;
    glm::dmat4 model_view_matrix(1.0), projection_matrix(1.0);

    //glGetDoublev( GL_MODELVIEW_MATRIX,  glm::value_ptr(model_view_matrix) );
    glGetDoublev( GL_PROJECTION_MATRIX, glm::value_ptr(projection_matrix) );
    glGetIntegerv( GL_VIEWPORT, (int*)&viewport );

    size_t data_count = 0;
    
    unsigned char rgba[4*width*height];

    // Convert the camera position to a double vector using homogeneous coordinates.
    glm::dvec4 d_camera_pos = glm::dvec4((double)(camera_pos.x), (double)(camera_pos.y), (double)(camera_pos.z), 0.0);
    glm::dmat4 model_to_camera_coordinates = glm::rotate(glm::dmat4(1.0), M_PI, glm::dvec3(0.0, 1.0, 0.0));

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)(rgba));

    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {
        size_t pos = 4*(j*height+i);
        
        int gb = rgba[pos + 1] * 255 + rgba[pos + 2];
        if (gb == 0) continue;
        double t = gb / 65536.0;
        //double d = t * (far_plane - real_near_plane) + real_near_plane;

        glm::dvec4 position;
	position.w = 0.0;
        gluUnProject(i, j, t, glm::value_ptr(model_view_matrix), glm::value_ptr(projection), (int*)&viewport, &(position[0]), &(position[1]), &(position[2]) );

	// Transform back into camera coordinates
	glm::dvec4 position_cc = model_to_camera_coordinates * position; 
        
        data[data_count]   =  (float)position_cc[0];
        data[data_count+1] =  (float)position_cc[1];
        data[data_count+2] =  (float)position_cc[2];
        data[data_count+3] =  rgba[pos + 0] / 256.0;

        data_count += 4;
      }
    }

    return data_count;    
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

    float* data       = new float[4*width*height + 4];
    size_t data_count = write_point_cloud(data, width, height);
    
    std::ofstream out(filename.c_str());
    
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

  float get_camera_z() const { return camera_pos.z; }
  glm::vec4 get_camera_pos() const { return camera_pos; }
  glm::vec4 get_camera_dir() const { return camera_dir; }
  float get_near_plane() const { return real_near_plane; }
  float get_far_plane() const { return far_plane; }

private:
  Mesh mesh;
  float scale_factor;

  glm::dmat4 projection;
  glm::vec4 camera_pos;
  glm::vec4 camera_dir;
  GLfloat near_plane_bound;
  GLfloat real_near_plane;
  GLfloat far_plane;
};

#endif
