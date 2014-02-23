//
// Created by John Woods on 2/17/14.
//


#ifndef __MESH_H_
#define __MESH_H_

#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define INVALID_OGL_VALUE 0xFFFFFFFF

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
    Mesh() { }

    ~Mesh() { clear(); }

    bool load_mesh(const std::string& filename) {
      // release the previously loaded mesh if it exists
      clear();

      bool ret = false;
      Assimp::Importer importer;

      const aiScene* scene = importer.ReadFile(filename.c_str(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FixInfacingNormals); //| aiProcess_FixInfacingNormals);

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
          textures[material_index]->bind(GL_TEXTURE0, shader_program);
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

private:
    bool init_from_scene(const aiScene* scene, const std::string& filename) {
      entries.resize(scene->mNumMeshes);
      textures.resize(scene->mNumMaterials);

      // Initialize the meshes in the scene one by one
      for (unsigned int i = 0; i < entries.size(); ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        init_mesh(i, mesh);
      }

      return init_materials(scene, filename);
    }

    void init_mesh(unsigned int index, const aiMesh* mesh) {
      entries[index].material_index = mesh->mMaterialIndex;

      std::vector<Vertex> vertices;
      std::vector<unsigned int> indices;

      const aiVector3D zero_3d(0.0, 0.0, 0.0);

      for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        const aiVector3D* pos    = &(mesh->mVertices[i]);
        const aiVector3D* normal = &(mesh->mNormals[i]);
        const aiVector3D* texture_coord = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][i]) : &zero_3d;

        Vertex v(glm::vec3(pos->x, pos->y, pos->z),
            glm::vec2(texture_coord->x, texture_coord->y),
            glm::vec3(normal->x, normal->y, normal->z));

        vertices.push_back(v);
      }

      // Add vertices for each face
      for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices != 3) {
          std::cerr << "Face has " << face.mNumIndices << " indices; skipping" << std::endl;
          continue;
        }
        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
      }

      // Create index buffer.
      entries[index].init(vertices, indices);
    }



    bool init_materials(const aiScene* scene, const std::string& filename) {
      // Extract the directory part from the file name
      std::string::size_type slash_index = filename.find_last_of("/");
      std::string dir;

      if (slash_index == std::string::npos)   dir = ".";
      else if (slash_index == 0)              dir = "/";
      else                                    dir = filename.substr(0, slash_index);
      bool ret = true;

      if (scene->HasTextures())
        std::cerr << "Scene has textures!" << std::endl;

      for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        std::cerr << "Loading material " << i+1 << " of " << scene->mNumMaterials << std::endl;
        const aiMaterial* material = scene->mMaterials[i];

        //for (size_t i = 0; i <= static_cast<size_t>(aiTextureType_UNKNOWN); ++i) {
        //  std::cerr << "texture count for " << i << " is " << material->GetTextureCount(static_cast<aiTextureType>(i)) << std::endl;
        //}

        textures[i] = NULL;
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {

          std::cerr << "Texture count = " << material->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
          aiString path;
          if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            std::string full_path = dir + "/" + path.data;
            std::cerr << "Registering texture from " << full_path << std::endl;
            textures[i] = new Texture(GL_TEXTURE_2D, full_path.c_str());

            if (!textures[i]->load()) {
              std::cerr << "Error loading texture '" << full_path << "'" << std::endl;
              delete textures[i];
              textures[i] = NULL;
              ret = false;
            }
          }
        }

        // This code handles most of our models --- which have no textures:
        if (!textures[i]) {
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
          textures[i] = new Texture(GL_TEXTURE_2D, "./white.png");
          ret = textures[i]->load();
        }
      }

      return ret;
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
};


#endif //__MESH_H_
