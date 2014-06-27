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


#ifndef __MESH_H_
#define __MESH_H_

#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define INVALID_OGL_VALUE 0xFFFFFFFF
//#define AI_CONFIG_PP_RVC_FLAGS  aiComponent_NORMALS

#include "texture.h"


// Use this struct to represent vertex coordinates, texture coordinates, and the normal coordinates for this vertex.
struct Vertex
{
  glm::vec3 pos;
  glm::vec2 tex;
  glm::vec3 normal;

  Vertex() {}

  Vertex(const glm::vec3& pos_, const glm::vec2& tex_, const glm::vec3& normal_)
  {
    pos    = pos_;
    tex    = tex_;
    normal = normal_;
  }
};


// Ganked from: http://ogldev.atspace.co.uk/www/tutorial22/tutorial22.html
class Mesh {
public:
  Mesh() : min_extremities(0.0f,0.0f,0.0f), max_extremities(0.0f,0.0f,0.0f) { }

  ~Mesh() { clear(); }


  glm::vec3 dimensions() const {
    return max_extremities - min_extremities;
  }


  bool load_mesh(const std::string& filename) {
    // release the previously loaded mesh if it exists
    clear();

    bool ret = false;
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filename.c_str(), aiProcess_Triangulate );

    if (scene)    ret = init_from_scene(scene, filename);
    else std::cerr << "Error parsing '" << filename << "': " << importer.GetErrorString() << std::endl;

    return ret;
  }

  void render(Shader* shader_program) {
    shader_program->bind();

    GLuint position_loc = glGetAttribLocation(shader_program->id(), "position");
    GLuint tex_loc      = glGetAttribLocation(shader_program->id(), "tex");
    GLuint normal_loc   = glGetAttribLocation(shader_program->id(), "normal");

    glEnableVertexAttribArray(position_loc);
    glEnableVertexAttribArray(tex_loc);
    glEnableVertexAttribArray(normal_loc);

    for (size_t i = 0; i < entries.size(); ++i) {
      glBindBuffer(GL_ARRAY_BUFFER, entries[i].vb);

      // I think this tells it where to look for the vertex information we've loaded.
      glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
      glVertexAttribPointer(tex_loc,      2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12); // starts at 12 because 3 floats for position before 2 floats for normal
      glVertexAttribPointer(normal_loc,   3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20); // makes room for 5 floats

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entries[i].ib);

      const size_t material_index = entries[i].material_index;


      if (material_index < textures.size() && textures[material_index]) {
        textures[material_index]->bind(shader_program);
      }

      //glColor4f(1.0, 1.0, 1.0, 1.0);
      glDrawElements(GL_TRIANGLES, entries[i].num_indices, GL_UNSIGNED_INT, 0);
    }

    GLenum error_check_value = glGetError();
    if (error_check_value != GL_NO_ERROR) {
      std::cerr << "Could not create a VBO: " << gluErrorString(error_check_value) << std::endl;
    }

    glDisableVertexAttribArray(position_loc);
    glDisableVertexAttribArray(tex_loc);
    glDisableVertexAttribArray(normal_loc);

    shader_program->unbind();
  }

  /*
   * Get the centroid of the object, an average of the positions of all vertices.
   *
   */
  glm::vec3 centroid() const {
    return centroid_;
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

  struct MeshEntry {
      MeshEntry() {
        vb = INVALID_OGL_VALUE;
        ib = INVALID_OGL_VALUE;
        num_indices = 0;
        material_index = INVALID_MATERIAL;
      }

      ~MeshEntry() {
        if (vb != INVALID_OGL_VALUE) glDeleteBuffers(1, &vb);
        if (ib != INVALID_OGL_VALUE) glDeleteBuffers(1, &ib);
      }

      void init(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
        num_indices = indices.size();

        glGenBuffers(1, &vb);
        glBindBuffer(GL_ARRAY_BUFFER, vb);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

        glGenBuffers(1, &ib);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * num_indices, &indices[0], GL_STATIC_DRAW);
      }

      GLuint vb;
      GLuint ib;
      size_t num_indices;
      size_t material_index;
  };


  std::vector<MeshEntry> entries;
  std::vector<Texture*> textures;
  glm::vec3 min_extremities, max_extremities, centroid_;
};


#endif //__MESH_H_
