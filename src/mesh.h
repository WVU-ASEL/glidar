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

// This file based loosely on a GPLv3 example given by Etay Meiri. I believe I've
// changed it sufficiently that it's no longer a derivative work, but please feel
// free to contact me if you feel I've violated the spirit of the GPL.

#ifndef MESH_H
# define MESH_H

#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <glm/gtx/projection.hpp> // glm::proj
#include <glm/gtx/string_cast.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <flann/flann.hpp>
#include <flann/algorithms/kdtree_single_index.h>

#define INVALID_OGL_VALUE 0xFFFFFFFF
//#define AI_CONFIG_PP_RVC_FLAGS  aiComponent_NORMALS

#include "texture.h"

const size_t MAX_LEAF_SIZE = 16;
const float MIN_NEAR_PLANE = 0.01; // typically meters, but whatever kind of distance units you're using for your world.


// Use this struct to represent vertex coordinates, texture coordinates, and the normal coordinates for this vertex.
struct Vertex
{
  glm::vec3 pos;
  glm::vec2 diffuse_tex;
  glm::vec2 specular_tex;
  glm::vec3 normal;

  Vertex() {}

  Vertex(const glm::vec3& pos_, const glm::vec2& dtex_, const glm::vec2& stex_, const glm::vec3& normal_)
  : pos(pos_), diffuse_tex(dtex_), specular_tex(stex_), normal(normal_)
  {  }

  float x() const { return pos.x; }
  float y() const { return pos.y; }
  float z() const { return pos.z; }
};


// Ganked from: http://ogldev.atspace.co.uk/www/tutorial22/tutorial22.html
class Mesh {
public:
  enum Attribute {
    ATTR_POSITION,
    ATTR_DIFFUSE_TEXCOORDS,
    ATTR_SPECULAR_TEXCOORDS,
    ATTR_NORMAL
  };


  Mesh() : min_extremities(0.0f,0.0f,0.0f), max_extremities(0.0f,0.0f,0.0f) { }

  
  ~Mesh() {
    clear();
  }


  glm::vec3 dimensions() const {
    return max_extremities - min_extremities;
  }


  bool load_mesh(const std::string& filename) {
    // release the previously loaded mesh if it exists
    clear();

    bool ret = false;
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filename.c_str(), aiProcess_Triangulate | aiProcess_GenNormals );

    if (scene)    ret = init_from_scene(scene, filename);
    else std::cerr << "Error parsing '" << filename << "': " << importer.GetErrorString() << std::endl;

    return ret;
  }

  
  void render(Shader* shader_program) {

    shader_program->bind();

    check_gl_error();

    GLuint position_loc     = glGetAttribLocation(shader_program->id(), "position");
    GLuint diffuse_tex_loc  = glGetAttribLocation(shader_program->id(), "diffuse_tex");
    GLuint specular_tex_loc = glGetAttribLocation(shader_program->id(), "specular_tex");
    GLuint normal_loc       = glGetAttribLocation(shader_program->id(), "normal");

    check_gl_error();

    glEnableVertexAttribArray(position_loc);
    check_gl_error();
    glEnableVertexAttribArray(diffuse_tex_loc);
    check_gl_error();
    glEnableVertexAttribArray(specular_tex_loc);
    check_gl_error();
    glEnableVertexAttribArray(normal_loc);

    check_gl_error();

    for (size_t i = 0; i < entries.size(); ++i) {
      glBindBuffer(GL_ARRAY_BUFFER, entries[i].vb);

      // I think this tells it where to look for the vertex information we've loaded.
      glVertexAttribPointer(position_loc,           3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
      glVertexAttribPointer(diffuse_tex_loc,  2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12); // starts at 12 because 3 floats for position before 2 floats for normal
      glVertexAttribPointer(specular_tex_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);
      glVertexAttribPointer(normal_loc,             3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)28); // makes room for 7 floats

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entries[i].ib);

      const size_t material_index = entries[i].material_index;


      if (material_index < textures.size() && textures[material_index]) {
        textures[material_index]->bind(shader_program);
      }

      //glColor4f(1.0, 1.0, 1.0, 1.0);
      glDrawElements(GL_TRIANGLES, entries[i].num_indices, GL_UNSIGNED_INT, 0);
    }

    check_gl_error();

    glDisableVertexAttribArray(position_loc);
    glDisableVertexAttribArray(diffuse_tex_loc);
    glDisableVertexAttribArray(specular_tex_loc);
    glDisableVertexAttribArray(normal_loc);

    check_gl_error();

    shader_program->unbind();
  }

  /** Get the centroid of the object, an average of the positions of all vertices.
   *
   */
  glm::vec3 centroid() const {
    return centroid_;
  }

  /** Find the nearest point among all the meshes to some point p.
   *
   * @param[in] query point.
   * @param[out] result point.
   *
   * \returns a distance.
   */
  float nearest_point(const glm::vec4& p, glm::vec4& result) const {
    float d = entries[0].nearest_point(p, result);

    for (size_t i = 1; i < entries.size(); ++i) {
      glm::vec4 tmp_result;
      float tmp_distance;

      tmp_distance = entries[i].nearest_point(p, tmp_result);

      if (tmp_distance < d) {
	d = tmp_distance;
	result = tmp_result;
      }
    }

    return d;
  }

  /** Find the ideal location of the near plane.
   *
   * @param[in] model coordinate matrix.
   * @param[in] camera position.
   *
   * \returns The distance from the camera to the ideal near plane if we didn't mind the plane intersecting the nearest point.
   */
  float near_plane_bound(const glm::mat4& model_to_object_coords, const glm::vec4& camera_pos) const {
    glm::vec4 nearest;

    // Find the nearest point in the mesh.
    nearest_point(camera_pos, nearest);
    nearest.w = 0.0;

    float bound = (model_to_object_coords * (camera_pos - nearest)).z;

    if (bound <= 0) {
      std::cerr << "WARNING: Nearest point on object is behind the sensor, which makes for an invalid near plane setting. Using MIN_NEAR_PLANE="
                << MIN_NEAR_PLANE << " distance units for the bound. Actual near plane will be slightly closer, depending on your value for NEAR_PLANE_FACTOR." 
                << std::endl;
      bound = MIN_NEAR_PLANE;
    }

    return bound;
  }


  /** Find the ideal location of the far plane.
   *
   * @param[in] model coordinate matrix.
   * @param[in] camera position.
   *
   * \returns The distance from the camera to the ideal far plane if we didn't mind the plane intersecting the farthest point.
   */
  float far_plane_bound(const glm::mat4& model_to_object_coords, const glm::vec4& camera_pos) const {
    // Find the farthest point in the mesh
    glm::vec4 negative_camera_pos(-camera_pos);
    glm::vec4 farthest;
    nearest_point(negative_camera_pos, farthest);
    farthest.w = 0.0;
    return (model_to_object_coords * (camera_pos - farthest)).z;
  }

private:
  void init_mesh(const aiScene* scene, const aiMesh* mesh, size_t index);
  bool init_materials(const aiScene* scene, const std::string& filename);

  bool init_from_scene(const aiScene* scene, const std::string& filename) {
    entries.resize(scene->mNumMeshes);
    textures.resize(scene->mNumMaterials);

    std::cout << "Reading " << entries.size() << " meshes" << std::endl;

    // Initialize the meshes in the scene one by one
    for (size_t i = 0; i < entries.size(); ++i) {
      const aiMesh* mesh = scene->mMeshes[i];
      init_mesh(scene, mesh, i);
    }

    return init_materials(scene, filename);
  }

  void clear() {
    for (size_t i = 0; i < textures.size(); ++i) {
      delete textures[i];
      textures[i] = NULL;
    }
  }

#define INVALID_MATERIAL 0xFFFFFFFF

  class MeshEntry {
  public:
    MeshEntry()
      : vb(INVALID_OGL_VALUE), 
        ib(INVALID_OGL_VALUE), 
        num_indices(0),
        material_index(INVALID_MATERIAL),
        xyz_data(NULL),
        xyz(NULL),
        kdtree(NULL),
        flann_epsilon(0.0),
        flann_sorted(true),
        flann_checks(32)
    { }

    ~MeshEntry() {
      if (vb != INVALID_OGL_VALUE) glDeleteBuffers(1, &vb);
      if (ib != INVALID_OGL_VALUE) glDeleteBuffers(1, &ib);

      // Delete the space allocated within xyz, then delete xyz container, then delete the kdtree.
      delete [] xyz_data;
      delete xyz;
      delete kdtree;
    }

    void init(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
      num_indices = indices.size();

      glGenBuffers(1, &vb);
      glBindBuffer(GL_ARRAY_BUFFER, vb);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

      glGenBuffers(1, &ib);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * num_indices, &indices[0], GL_STATIC_DRAW);

      // Copy the xyz coordinates from vertices into the xyz array.
      xyz_data = new float[vertices.size() * 3];
      for (size_t i = 0; i < vertices.size(); ++i) {
	xyz_data[i*3+0] = vertices[i].x();
	xyz_data[i*3+1] = vertices[i].y();
        xyz_data[i*3+2] = vertices[i].z();
      }
      // Create the matrix
      xyz = new flann::Matrix<float>(xyz_data, vertices.size(), 3);

      // Create a k-D tree, which we will use to find the near plane no matter how the object is rotated.
      kdtree = new flann::KDTreeSingleIndex<flann::L2_Simple<float> >(*xyz, flann::KDTreeSingleIndexParams(MAX_LEAF_SIZE));
      kdtree->buildIndex();
    }

    /** Returns the nearest point in model coordinates to a given point.
     *
     * @param[in] The query point.
     * @param[out] The nearest point found to the query point.
     *
     * \returns The distance from the query point to the nearest mesh point.
     */
    float nearest_point(const glm::vec4& p, glm::vec4& result) const {
      flann::Matrix<float> query(const_cast<float*>(static_cast<const float*>(glm::value_ptr(p))), 1, 3);

      // Result matrices
      int index;
      float distance;
      flann::Matrix<int> indices(&index, 1, 1);
      flann::Matrix<float> distances(&distance, 1, 1);

      flann::SearchParams search_parameters;
      search_parameters.eps = flann_epsilon;
      search_parameters.sorted = flann_sorted;
      search_parameters.checks = flann_checks;


      kdtree->knnSearch(query, indices, distances, 1, search_parameters);

      result.x = xyz_data[index*3+0];
      result.y = xyz_data[index*3+1];
      result.z = xyz_data[index*3+2];
      result.w = 0.0;

      return distance;
    }

    GLuint vb;
    GLuint ib;
    size_t num_indices;
    size_t material_index;

    float* xyz_data;
    flann::Matrix<float>* xyz;
    flann::KDTreeSingleIndex<flann::L2_Simple<float> >* kdtree;
    float flann_epsilon;
    bool  flann_sorted;
    int   flann_checks;
  };


  std::vector<MeshEntry> entries;
  std::vector<Texture*> textures;
  glm::vec3 min_extremities, max_extremities, centroid_;
};


#endif // MESH_H
