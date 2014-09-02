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
  glm::vec2 diffuse_tex;
  glm::vec2 specular_tex;
  glm::vec3 normal;

  Vertex() {}

  Vertex(const glm::vec3& pos_, const glm::vec2& dtex_, const glm::vec2& stex_, const glm::vec3& normal_)
  : pos(pos_), diffuse_tex(dtex_), specular_tex(stex_), normal(normal_)
  {  }
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

  ~Mesh() { clear(); }


  glm::vec3 dimensions() const {
    return max_extremities - min_extremities;
  }


  bool load_mesh(const std::string& filename) {
    // release the previously loaded mesh if it exists
    clear();

    bool ret = false;
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filename.c_str(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals );

    if (scene)    ret = init_from_scene(scene, filename);
    else std::cerr << "Error parsing '" << filename << "': " << importer.GetErrorString() << std::endl;

    return ret;
  }

  void render(Shader* shader_program) {

    shader_program->bind();

    check_gl_error();

/*    glBindAttribLocation(shader_program->id(), ATTR_POSITION, "position");
    glBindAttribLocation(shader_program->id(), ATTR_DIFFUSE_TEXCOORDS, "diffuse_tex");
    glBindAttribLocation(shader_program->id(), ATTR_SPECULAR_TEXCOORDS, "specular_tex");
    glBindAttribLocation(shader_program->id(), ATTR_NORMAL, "normal");*/
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
