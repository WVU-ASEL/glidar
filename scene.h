#ifndef SCENE_H
# define SCENE_H

#include "mesh.h"

const float ASPECT_RATIO = 1.0;

class Scene {
public:
  Scene(const std::string& filename, float scale_factor_ = 1.0)
  : scale_factor(scale_factor_)
  {
    mesh.load_mesh(filename);
  }


  void setup() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    glEnable(GL_TEXTURE_2D);

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    std::cerr << "Initial position is (0,0,-1000) with clipping plane 1.0, 1250.0" << std::endl;
    std::cerr << "Box is a 200 x 200 x 200 meter cube." << std::endl;
    gluPerspective(20.0f, ASPECT_RATIO, 1.0, 1250.0);
    glTranslatef(0.0, 0.0, -1000.0);

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

  void render() {
    // clear window with the current clearing color, and clear the depth buffer
    glClear(GL_COLOR_BUFFER_BIT); // | GL_DEPTH_BUFFER_BIT);

    // Setup model view matrix: All future transformations will affect our models (what we draw).
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // For debugging purposes, let's make sure we can see a box.
    render_box();

    glPushMatrix();

    glColor3ub(255, 255, 0);

    // Draw some test triangles. These work just fine if uncommented.
/*    glScalef(10.0, 10.0, 10.0);
    glBegin(GL_TRIANGLES);
      glVertex3f(-1.0, 1.0, -1.0);
      glVertex3f(0.0, 5.0,  5.0);
      glVertex3f(1.0, 1.0, -1.0);
    glEnd();*/

    // Scale only the mesh.
    glScalef(scale_factor, scale_factor, scale_factor);

    glColor3ub(1,1,1);

    // Render the mesh.
    mesh.render();

    glPopMatrix();

    // flush drawing commands
    glFlush();
  }

private:
  Mesh mesh;
  float scale_factor;
};

#endif