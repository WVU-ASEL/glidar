#include "mesh.h"


void Mesh::init_mesh(unsigned int index, const aiMesh* mesh) {

  std::cout << "Loading mesh named '" << mesh->mName.C_Str() << "'" << std::endl;

  entries[index].material_index = mesh->mMaterialIndex;

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  const aiVector3D zero_3d(0.0, 0.0, 0.0);


  // initialize our dimension trackers.
  if (mesh->mNumVertices != 0) {
    min_extremities.x = mesh->mVertices[0].x;
    min_extremities.y = mesh->mVertices[0].y;
    min_extremities.z = mesh->mVertices[0].z;
    max_extremities = min_extremities;
  }


  if (mesh->mNumBones) {
    for (size_t i = 0; i < mesh->mNumBones; ++i) {
      const aiBone *bone = mesh->mBones[i];
      const aiMatrix4x4& bone_matrix = bone->mOffsetMatrix;
      aiMatrix3x3 bone_normal_matrix = aiMatrix3x3(bone_matrix);

      std::cout << "Bone '" << bone->mName.C_Str() << "' includes " << bone->mNumWeights << " vertices:" << std::endl;
      for (size_t j = 0; j < bone->mNumWeights; ++j) {
        std::cout << ' ' << bone->mWeights[j].mVertexId;
      }
      std::cout << std::endl;

      for (size_t j = 0; j < bone->mNumWeights; ++j) {
        const aiVertexWeight *vertex_weight = &(bone->mWeights[j]);
        size_t v = (size_t)(vertex_weight->mVertexId);
        float w = vertex_weight->mWeight;

        const aiVector3D *src_pos = &(mesh->mVertices[v]);
        const aiVector3D *src_normal = &(mesh->mNormals[v]);
        const aiVector3D *texture_coord = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][v]) : &zero_3d;

        aiVector3D pos = w * (bone_matrix * (*src_pos));
        aiVector3D normal = w * (bone_normal_matrix * (*src_normal));

        // Find the extremities of this mesh so we can get a measurement for the object in object units.
        if (pos.x < min_extremities.x) min_extremities.x = pos.x;
        else if (pos.x > max_extremities.x) max_extremities.x = pos.x;

        if (pos.y < min_extremities.y) min_extremities.y = pos.y;
        else if (pos.y > max_extremities.y) max_extremities.y = pos.y;

        if (pos.z < min_extremities.z) min_extremities.z = pos.z;
        else if (pos.z > max_extremities.z) max_extremities.z = pos.z;

        Vertex vertex(glm::vec3(pos.x, pos.y, pos.z),
            glm::vec2(texture_coord->x, texture_coord->y),
            glm::vec3(normal.x, normal.y, normal.z));

        // Accumulate the centroid_ of the object.
        centroid_ += vertex.pos;

        vertices.push_back(vertex);
      }
    }
  } else {
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
      const aiVector3D* pos    = &(mesh->mVertices[i]);
      const aiVector3D* normal = &(mesh->mNormals[i]);
      const aiVector3D* texture_coord = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][i]) : &zero_3d;

      // Find the extremities of this mesh so we can get a measurement for the object in object units.
      if (pos->x < min_extremities.x)       min_extremities.x = pos->x;
      else if (pos->x > max_extremities.x)  max_extremities.x = pos->x;

      if (pos->y < min_extremities.y)       min_extremities.y = pos->y;
      else if (pos->y > max_extremities.y)  max_extremities.y = pos->y;

      if (pos->z < min_extremities.z)       min_extremities.z = pos->z;
      else if (pos->z > max_extremities.z)  max_extremities.z = pos->z;

      Vertex v(glm::vec3(pos->x, pos->y, pos->z),
          glm::vec2(texture_coord->x, texture_coord->y),
          glm::vec3(normal->x, normal->y, normal->z));

      // Accumulate the centroid_ of the object.
      centroid_ += v.pos;

      vertices.push_back(v);
    }
  }

  centroid_ /= mesh->mNumVertices;

  // Add vertices for each face
  for (size_t i = 0; i < mesh->mNumFaces; ++i) {
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


bool Mesh::init_materials(const aiScene* scene, const std::string& filename) {
  // Extract the directory part from the file name
  std::string::size_type slash_index = filename.find_last_of("/");
  std::string dir;

  if (slash_index == std::string::npos)   dir = ".";
  else if (slash_index == 0)              dir = "/";
  else                                    dir = filename.substr(0, slash_index);
  bool ret = true;

  if (scene->HasTextures())
    std::cerr << "Scene has textures!" << std::endl;

  for (size_t i = 0; i < scene->mNumMaterials; ++i) {
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
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      textures[i] = new Texture(GL_TEXTURE_2D, "./white.png");
      ret = textures[i]->load();
    }
  }

  return ret;
}
