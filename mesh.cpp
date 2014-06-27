#include "mesh.h"

void Mesh::init_mesh(const aiScene* scene, const aiMesh* mesh, size_t index) {

  std::cout << "Loading mesh named '" << mesh->mName.C_Str() << "'" << std::endl;

  entries[index].material_index = mesh->mMaterialIndex;

  if (!mesh->HasNormals()) {
    std::cerr << "Mesh has no normals!" << std::endl;
  } else {
    if (mesh->mNumVertices > 0)
      std::cerr << "First normal:" << mesh->mNormals[0].x << "," << mesh->mNormals[0].y << "," << mesh->mNormals[0].z << std::endl;
  }

  // Create vectors to store the transformed position and normals; initialize them to the origin.
  std::vector<aiVector3D> final_pos(mesh->mNumVertices),
                          final_normal(mesh->mNumVertices);
  for (size_t i = 0; i < mesh->mNumVertices; ++i) {
    final_pos[i] = final_normal[i] = aiVector3D(0.0, 0.0, 0.0);
  }


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
    std::vector<aiMatrix4x4> bone_matrices(mesh->mNumBones);

    // Calculate bone matrices.
    for (size_t i = 0; i < mesh->mNumBones; ++i) {
      const aiBone* bone = mesh->mBones[i];
      bone_matrices[i] = bone->mOffsetMatrix;

      std::cout << "Bone '" << bone->mName.C_Str() << "' includes " << bone->mNumWeights << " vertices:" << std::endl;
      for (size_t j = 0; j < bone->mNumWeights; ++j) {
        std::cout << ' ' << bone->mWeights[j].mVertexId;
      }
      std::cout << std::endl;

      const aiNode* node = scene->mRootNode->FindNode(bone->mName.C_Str());
      const aiNode* temp_node = node;
      while (temp_node != NULL) {
        bone_matrices[i] = temp_node->mTransformation * bone_matrices[i];
        temp_node = temp_node->mParent;
      }
    }

    // Update vertex positions according to calculated matrices
    for (size_t i = 0; i < mesh->mNumBones; ++i) {
      const aiBone* bone = mesh->mBones[i];
      aiMatrix3x3 normal_matrix = aiMatrix3x3(bone_matrices[i]);

      for (size_t j = 0; j < bone->mNumWeights; ++j) {
        const aiVertexWeight *vertex_weight = &(bone->mWeights[j]);
        size_t v = (size_t)(vertex_weight->mVertexId);
        float  w = vertex_weight->mWeight;

        const aiVector3D *src_pos = &(mesh->mVertices[v]);
        const aiVector3D *src_normal = &(mesh->mNormals[v]);

        final_pos[v]    += w * (bone_matrices[i] * (*src_pos));
        final_normal[v] += w * (normal_matrix * (*src_normal));
      }

      std::cout << "bone " << i << ":" << std::endl;
      std::cout << bone_matrices[i].a1 << ' ' << bone_matrices[i].a2 << ' ' << bone_matrices[i].a3 << ' ' << bone_matrices[i].a4 << std::endl;
      std::cout << bone_matrices[i].b1 << ' ' << bone_matrices[i].b2 << ' ' << bone_matrices[i].b3 << ' ' << bone_matrices[i].b4 << std::endl;
      std::cout << bone_matrices[i].c1 << ' ' << bone_matrices[i].c2 << ' ' << bone_matrices[i].c3 << ' ' << bone_matrices[i].c4 << std::endl;
      std::cout << bone_matrices[i].d1 << ' ' << bone_matrices[i].d2 << ' ' << bone_matrices[i].d3 << ' ' << bone_matrices[i].d4 << std::endl;
    }

    // Add each updated vertex and calculate its extremities.
    for (size_t i = 0; i < mesh->mNumVertices; ++i) {
      const aiVector3D *texture_coord = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][i]) : &zero_3d;

      // Find the extremities of this mesh so we can get a measurement for the object in object units.
      if (final_pos[i].x < min_extremities.x) min_extremities.x = final_pos[i].x;
      else if (final_pos[i].x > max_extremities.x) max_extremities.x = final_pos[i].x;

      if (final_pos[i].y < min_extremities.y) min_extremities.y = final_pos[i].y;
      else if (final_pos[i].y > max_extremities.y) max_extremities.y = final_pos[i].y;

      if (final_pos[i].z < min_extremities.z) min_extremities.z = final_pos[i].z;
      else if (final_pos[i].z > max_extremities.z) max_extremities.z = final_pos[i].z;


      Vertex vertex(glm::vec3(final_pos[i].x, final_pos[i].y, final_pos[i].z),
          glm::vec2(texture_coord->x, texture_coord->y),
          glm::vec3(final_normal[i].x, final_normal[i].y, final_normal[i].z));

      std::cout << "Adding vertex " << i << ": " << final_pos[i].x << "," << final_pos[i].y << "," << final_pos[i].z;
      std::cout << "\t" << final_normal[i].x << "," << final_normal[i].y << "," << final_normal[i].z << std::endl;
      std::cout << "  was: " << mesh->mVertices[i].x << "," << mesh->mVertices[i].y << "," << mesh->mVertices[i].z << '\t';
      std::cout << mesh->mNormals[i].x << "," << mesh->mNormals[i].y << "," << mesh->mNormals[i].z << std::endl;

      // Accumulate the centroid_ of the object.
      centroid_ += vertex.pos;

      vertices.push_back(vertex);
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
  std::string::size_type slash_index = filename.find_first_of("/");
  std::string dir;

  if (slash_index == std::string::npos)   dir = ".";
  else if (slash_index == 0)              dir = "/";
  else                                    dir = filename.substr(0, slash_index);
  bool ret = true;

  if (scene->HasTextures())
    std::cerr << "Scene has textures!" << std::endl;

  for (size_t i = 0; i < scene->mNumMaterials; ++i) {
    textures[i] = NULL;
    std::cerr << "Loading material " << i+1 << " of " << scene->mNumMaterials << std::endl;
    const aiMaterial* material = scene->mMaterials[i];


    std::vector<std::string> texture_filenames(2);
    texture_filenames[0] = "./white.png";
    texture_filenames[1] = "./white.png";

    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
      std::cerr << "Diffuse texture count = " << material->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
      aiString path;

      if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
        std::string full_path = dir + "/" + path.data;
        std::cerr << "Registering diffuse texture from " << full_path.c_str() << std::endl;

        texture_filenames[0] = std::string(full_path.c_str());
      }
    }


    if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) {
      std::cerr << "Specular texture count = " << material->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
      aiString path;

      if (material->GetTexture(aiTextureType_SPECULAR, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
        std::string full_path = dir + "/" + path.data;
        std::cerr << "Registering specular texture from " << full_path.c_str() << std::endl;

        texture_filenames[1] = std::string(full_path.c_str());

      }
    }

    textures[i] = new Texture(texture_filenames);
    textures[i]->load();
  }

  return ret;
}
