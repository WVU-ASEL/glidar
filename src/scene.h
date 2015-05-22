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

#ifndef SCENE_H
# define SCENE_H

#define GLM_FORCE_RADIANS

#include <Magick++.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/string_cast.hpp>
#include <cmath>
#include "mesh.h"
#include "quaternion.h"

#define _USE_MATH_DEFINES

const float ASPECT_RATIO = 1.0;
const float CAMERA_Y     = 0.05;
const unsigned int BOX_HALF_DIAGONAL = 174;

const double RADIANS_PER_DEGREE = M_PI / 180.0;
const float NEAR_PLANE_FACTOR = 0.99;
const float FAR_PLANE_FACTOR = 1.01;


/** Simple object and sensor OpenGL scene, which handles loading and rendering meshes, and also writing out point clouds.
 *
 * This file currently contains two independent render strategies -- one from before I started using quaternions and
 * one from after. They should produce the same result; I used the non-quaternion method to check the quaternion method.
 * Eventually, we want to remove the non-quaternion method altogether.
 *
 * Another thing: A lot of these functions don't need to be public. This program went through many, many iterations, and
 * over the course of those iterations, I made nearly everything public so I could play with things more easily.
 */
class Scene {
public:
  /** Constructor for the scene, which does most of the rendering work.
   *
   * This function sets near_plane_bound, real_near_plane, and far_plane, which end up not being used after initial setup.
   * Eventually they should be removed.
   *
   * @param[in] 3D model file to load.
   * @param[in] amount by which to scale the model we load.
   * @param[in] initial camera distance.
   */
  Scene(const std::string& filename, float scale_factor_, float camera_d_)
  : scale_factor(scale_factor_),
    projection(1.0),
    camera_d(camera_d_),
    near_plane_bound(camera_d_ - BOX_HALF_DIAGONAL),
    real_near_plane(std::max(MIN_NEAR_PLANE, camera_d_-BOX_HALF_DIAGONAL)),
    far_plane(camera_d_+BOX_HALF_DIAGONAL)
  {
    std::cerr << "camera_d = " << camera_d << std::endl;
    mesh.load_mesh(filename);

    glm::vec3 dimensions = mesh.dimensions();
    std::cerr << "Object dimensions as modeled: " << dimensions.x << '\t' << dimensions.y << '\t' << dimensions.z << std::endl;
    glm::vec3 centroid = mesh.centroid();
    std::cerr << "Center of object as modeled: " << centroid.x << '\t' << centroid.y << '\t' << centroid.z << std::endl;
  }


  /** Set OpenGL options.
   *
   */
  void gl_setup() {
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
    glDepthFunc(GL_LEQUAL);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glPolygonMode( GL_FRONT, GL_FILL );
  }



  /** Setup the perspective projection matrix, and figure out where to draw the near and far planes.
   *
   * @param[in] Field of view of the sensor.
   * @param[in] Model matrix inverse.
   * @param[in] View matrix.
   */
  void projection_setup(float fov, const glm::mat4& inverse_model, const glm::mat4& view_physics) {
    gl_setup();

    glm::mat4 inverse_view = glm::inverse(view_physics);
    glm::vec4 camera_pos_mc = inverse_model * inverse_view * glm::vec4(0.0, 0.0, 0.0, 1.0);
    glm::mat4 model = glm::inverse(inverse_model);
    
    near_plane_bound = mesh.near_plane_bound(model, camera_pos_mc);
    real_near_plane = near_plane_bound * NEAR_PLANE_FACTOR;
    far_plane = mesh.far_plane_bound(model, camera_pos_mc) * FAR_PLANE_FACTOR;

    projection = glm::perspective<float>(fov * M_PI / 180.0, ASPECT_RATIO, real_near_plane, far_plane);
  }


  /** Setup the lighting for the scene, which is basically just the LIDAR laser source.
   *
   * @param[in] the GLSL shader program.
   */
  void gl_setup_lighting(Shader* shader_program) {
    float light_position[] = {0.0, 0.0, 0.0, 1.0};
    float light_direction[] = {0.0, 0.0, 1.0, 0.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 10.0);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0001f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.00000001f);

    // Use an identity matrix for lighting.
    glm::mat4 light_matrix(1);
    GLint light_matrix_id = glGetUniformLocation(shader_program->id(), "LightModelViewMatrix");
    glUniformMatrix4fv(light_matrix_id, 1, false, static_cast<GLfloat*>(glm::value_ptr(light_matrix)));
  }


  /** Render the scene using the model and view matrices.
   *
   *Sets up the model view matrix, calls projection_setup, calculates a normal matrix.
   *
   * @param[in] the GLSL shader program.
   * @param[in] the field of view of the sensor.
   * @param[in] model matrix inverse.
   * @param[in] view matrix.
   */
  void render(Shader* shader_program, float fov, const glm::mat4& inverse_model_physics, const glm::mat4& view_physics) {
    glm::mat4 inverse_model = glm::scale(glm::mat4(1), glm::vec3(1.0/scale_factor, 1.0/scale_factor, 1.0/scale_factor)) *
      inverse_model_physics;
    projection_setup(fov, inverse_model, view_physics);

    // clear window with the current clearing color, and clear the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program->id());

    gl_setup_lighting(shader_program);

    glm::mat4 model = glm::inverse(inverse_model);

    GLint far_plane_id = glGetUniformLocation(shader_program->id(), "far_plane");
    GLint near_plane_id = glGetUniformLocation(shader_program->id(), "near_plane");

    glUniform1fv(far_plane_id, 1, &far_plane);
    glUniform1fv(near_plane_id, 1, &real_near_plane);

    GLint v_id = glGetUniformLocation(shader_program->id(), "ViewMatrix");
    glUniformMatrix4fv(v_id, 1, GL_FALSE, &view_physics[0][0]);
    
    glm::mat4 model_view = view_physics * model;
    GLint mv_id = glGetUniformLocation(shader_program->id(), "ModelViewMatrix");
    glUniformMatrix4fv(mv_id, 1, GL_FALSE, &model_view[0][0]);

    glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(model_view));
    GLint normal_id = glGetUniformLocation(shader_program->id(), "NormalMatrix");
    glUniformMatrix3fv(normal_id, 1, false, static_cast<GLfloat*>(glm::value_ptr(normal_matrix)));
    
    glm::mat4 model_view_projection = projection * model_view;
    
    GLint mvp_id = glGetUniformLocation(shader_program->id(), "ModelViewProjectionMatrix");
    glUniformMatrix4fv(mvp_id, 1, GL_FALSE, &model_view_projection[0][0]);

    mesh.render(shader_program);

    check_gl_error();

    glFlush();
  }
  

  /** Render the scene, calculating the view and inverse model matrices from attitudes (as quaternions) and translations.
   *
   * @param[in] the GLSL shader program.
   * @param[in] the field of view of the sensor.
   * @param[in] client object attitude.
   * @param[in] translation between the model and the sensor.
   * @param[in] sensor attitude.
   */
  void render(Shader* shader_program, float fov, const glm::dquat& model_q, const glm::dvec3& translate, const glm::dquat& camera_q) {
    glm::dquat flip = glm::angleAxis<double>(M_PI, glm::dvec3(0.0,1.0,0.0));
    
    glm::mat4 view          = glm::mat4(get_view_matrix(translate, camera_q));
    glm::mat4 inverse_model = glm::mat4(glm::mat4_cast(model_q * flip));

    render(shader_program, fov, inverse_model, view);
  }


  /** Write the pose information to a file.
   *
   * @param[in] file basename (without the extension).
   * @param[in] the pose information to write to the file.
   */
  void save_pose_metadata(const std::string& basename, const glm::dmat4& pose) {
    std::string filename = basename + ".transform";
    std::ofstream out(filename.c_str());
    out << std::scientific << std::setprecision(std::numeric_limits<double>::digits10 + 1)
        << pose[0][0] << ' ' << pose[1][0] << ' ' << pose[2][0] << ' ' << pose[3][0] << '\n'
        << pose[0][1] << ' ' << pose[1][1] << ' ' << pose[2][1] << ' ' << pose[3][1] << '\n'
        << pose[0][2] << ' ' << pose[1][2] << ' ' << pose[2][2] << ' ' << pose[3][2] << '\n'
        << pose[0][3] << ' ' << pose[1][3] << ' ' << pose[2][3] << ' ' << pose[3][3] << std::endl;
    out.close();
  }


  /** Write the translation and rotation information to a file.
   *
   * @param[in] file basename (without the extension).
   * @param[in] model attitude quaternion.
   * @param[in] model-sensor translation.
   * @param[in] sensor attitude quaternion.
   */
  void save_transformation_metadata(const std::string& basename, const glm::dquat& model_q, const glm::dvec3& translate, const glm::dquat& camera_q) {
    save_pose_metadata(basename, get_model_view_matrix_without_scaling(model_q, translate, camera_q));
  }


  /** Return the transformation metadata as a 4x4 homogeneous matrix (float).
   *
   * @param[in] model attitude quaternion.
   * @param[in] model-sensor translation.
   * @param[in] sensor attitude quaternion.
   */
  glm::mat4 get_pose(const glm::dquat& model_q, const glm::dvec3& translate, const glm::dquat& camera_q) {
    return glm::mat4(get_model_view_matrix_without_scaling(model_q, translate, camera_q));
  }
  

  /** Gets the model view matrix without the scaling component.
   *
   * Used for unprojecting when we get our point cloud out.
   *
   * @param[in] model rotation quaternion.
   * @param[in] model-sensor translation.
   * @param[in] sensor rotation quaternion.
   *
   * \returns a 4x4 homogeneous transformation (the model-view matrix).
   */
  glm::dmat4 get_model_view_matrix_without_scaling(const glm::dquat& model, const glm::dvec3& translate, const glm::dquat& camera) {
    glm::dquat flip = glm::angleAxis<double>(M_PI, glm::dvec3(0.0,1.0,0.0));
    return get_view_matrix(translate, camera) * glm::mat4_cast(model * flip);
  }

  
  /** Gets the model view matrix.
   *
   * @param[in] model rotation quaternion.
   * @param[in] model-sensor translation.
   * @param[in] sensor rotation quaternion.
   *
   * \returns a 4x4 homogeneous transformation (the model-view matrix).
   */  
  glm::dmat4 get_model_view_matrix(const glm::dquat& model, const glm::dvec3& translate, const glm::dquat& camera) {
    return get_view_matrix(translate, camera) * get_model_matrix(model);
  }

  /** Gets the view matrix.
   *
   * @param[in] model-sensor translation.
   * @param[in] sensor rotation quaternion.
   *
   * \returns a 4x4 homogeneous transformation (the model-view matrix).
   */  
  glm::dmat4 get_view_matrix(const glm::dvec3& translate, const glm::dquat& camera_q) {
    
    // Need to ensure that translate is 0,0,-z, and adjust the camera rotation to compensate.
    glm::dvec3 negative_z(0.0,0.0,-1.0);
    double rotation_angle = std::acos(glm::dot(negative_z, glm::normalize(translate)));
    glm::dvec3 rotation_axis = glm::cross(negative_z, translate);
    double rotation_axis_length = glm::length(rotation_axis);
    if (rotation_axis_length == 0)
      rotation_axis = glm::dvec3(0.0, 1.0, 0.0);
    else
      rotation_axis = rotation_axis / rotation_axis_length;
    
    glm::dquat rotation = glm::angleAxis<double>(-rotation_angle, rotation_axis);

    glm::dquat y_flip = glm::angleAxis<double>(M_PI, glm::dvec3(0.0,1.0,0.0));
    glm::dquat z_flip = glm::angleAxis<double>(M_PI/2.0, glm::dvec3(0.0,0.0,1.0));
    glm::dvec3 adjusted_translate = glm::mat3_cast(rotation) * translate;
    glm::dquat adjusted_camera_q = camera_q * z_flip * y_flip * rotation;
    
    return glm::mat4_cast(adjusted_camera_q) * glm::translate(glm::dmat4(1.0), adjusted_translate);
  }


  /** Gets the model transformation matrix.
   *
   * @param[in] model attitude quaternion.
   *
   * \returns a 4x4 homogeneous transformation (the model matrix).
   */  
  glm::dmat4 get_model_matrix(const glm::dquat& model) {
    glm::dquat flip = glm::angleAxis<double>(M_PI, glm::dvec3(0.0,1.0,0.0));
    return glm::mat4_cast(flip * model) * glm::scale(glm::dmat4(1.0), glm::dvec3(scale_factor, scale_factor, scale_factor));
  }


  /** Write only the data component of a point cloud to a buffer (no headers).
   *
   * Writes the point cloud to a buffer as x,y,z,i (in binary). Returns a size_t indicating the number of floating point 
   * entries written (note: not the number of bytes written).
   *
   * @param[in] model attitude quaternion.
   * @param[in] model-sensor translation.
   * @param[in] camera (sensor) attitude quaternion.
   * @param[out] the data buffer to which we wrote (pre-allocated by the calling function!)
   * @param[in] width of the sensor viewport
   * @parma[in] height of the sensor viewport
   *
   * \returns The total number of entries written to data (not the number of floats, mind you).
   */  
  size_t write_point_cloud(float* data, unsigned int width, unsigned int height) {
    glm::ivec4 viewport;
    glm::mat4 identity(1.0);
    
    glGetIntegerv( GL_VIEWPORT, (int*)&viewport );

    size_t data_count = 0;
    
    unsigned char rgba[4*width*height];

    glm::mat4 axis_flip = glm::scale(glm::mat4(1.0), glm::vec3(-1.0, 1.0, -1.0));

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)(rgba));

    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {
        size_t pos = 4*(j*height+i);
        
        int gb = rgba[pos + 1] * 255 + rgba[pos + 2];
        if (gb == 0) continue;
        double t = gb / 65536.0;
        double d = t * (far_plane - real_near_plane) + real_near_plane;

        glm::vec3 win(i,j,t);
	glm::vec4 position(glm::unProject(win, identity, projection, viewport), 0.0);

	// Transform back into camera coordinates
	position.z = -d; // Substitute in our correct distance value.
	glm::vec4 position_cc = axis_flip * position;


        data[data_count]   =  position_cc[0];
        data[data_count+1] =  position_cc[1];
        data[data_count+2] =  position_cc[2];
        data[data_count+3] =  rgba[pos + 0] / 256.0;
	
        data_count += 4;
      }
    }

    return data_count;    
  }



  /** Write the current color buffer as a PCD (point cloud file).
   *
   * @param[in] model attitude quaternion.
   * @param[in] model-sensor translation.
   * @param[in] sensor attitude quaternion.
   * @param[in] output file basename (without the extension, which will be .pcd)
   * @param[in] width of the viewport.
   * @param[in] height of the viewport.
   */
  void save_point_cloud(const std::string& basename, unsigned int width, unsigned int height) {
   std::string filename = basename + ".pcd";

    std::cerr << "Saving point cloud..." << std::endl;

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
  

  float get_near_plane() const { return real_near_plane; }
  float get_far_plane() const { return far_plane; }

private:
  Mesh mesh;
  float scale_factor;

  glm::mat4 projection;
  float camera_d;
  GLfloat near_plane_bound;
  GLfloat real_near_plane;
  GLfloat far_plane;
};

#endif
