#ifndef SCENE_H
# define SCENE_H

#include "mesh.h"

const float ASPECT_RATIO = 1.0;

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
    float far_plane = camera_z * 1.25;
    gluPerspective(20.0f, ASPECT_RATIO, 1.0, far_plane);
    glTranslatef(0.0, 0.0, -camera_z);

    glUseProgram(shader_program->id());
    GLint camera_z_id = glGetUniformLocation(shader_program->id(), "camera_z");
    GLint far_plane_id = glGetUniformLocation(shader_program->id(), "far_plane");

    std::cerr << "camera_z is now " << camera_z << std::endl;
    glUniform1fv(camera_z_id, 1, &camera_z);
    glUniform1fv(far_plane_id, 1, &far_plane);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }


  /*
   * Draws a simple box in green and red around the origin with dimensions 200 x 200 x 200.
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

  void render(Shader* shader_program, float x, float y, float z) {
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
    render_box();


    glPushMatrix();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

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

private:
  Mesh mesh;
  float scale_factor;
  float camera_z;
};

#endif