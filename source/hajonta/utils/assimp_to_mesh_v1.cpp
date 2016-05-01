#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <string>
#include <unordered_map>
#include <vector>

struct binary_format_v2
{
    uint32_t header_size;
    uint32_t vertices_offset;
    uint32_t vertices_size;
    uint32_t normals_offset;
    uint32_t normals_size;
    uint32_t texcoords_offset;
    uint32_t texcoords_size;
    uint32_t indices_offset;
    uint32_t indices_size;
    uint32_t num_triangles;
    uint32_t num_bones;
    uint32_t bone_names_offset;
    uint32_t bone_names_size;
    uint32_t bone_ids_offset;
    uint32_t bone_ids_size;
    uint32_t bone_weights_offset;
    uint32_t bone_weights_size;
    uint32_t bone_parent_offset;
    uint32_t bone_parent_size;
};

struct
BoneParent
{
    int32_t bone_id;
};

struct
BoneIds
{
    uint32_t bones[4];
};

struct
BoneWeights
{
    float weights[4];
};

void
process_nodes(aiNode *node, std::unordered_map<std::string, uint32_t> &bone_id_lookup, std::vector<std::string> &bone_names, std::vector<BoneParent> &bone_parents, binary_format_v2 &format, const char *prefix = 0, uint32_t length = 0, int32_t parent_bone = -1)
{
    char new_prefix[250];
    if (prefix)
    {
        snprintf(new_prefix, 250, "%s/%s", prefix, node->mName.C_Str());
    }
    else
    {
        snprintf(new_prefix, 250, "%s", node->mName.C_Str());
    }
    printf("%*s[%s]", length * 2, "", node->mName.C_Str());

    std::string bone_name(node->mName.data);
    if (!bone_id_lookup.count(bone_name))
    {
        char pointer[32];
        sprintf(pointer, "%p", node);
        bone_name += pointer;
        bone_id_lookup[bone_name] = (uint32_t)bone_id_lookup.size();
        // Bone name = length (1 byte) + name
        format.bone_names_size += 1 + (uint32_t)bone_name.length();
        bone_names.push_back(bone_name);
        format.bone_parent_size += (uint32_t)sizeof(BoneParent);
        bone_parents.push_back({parent_bone});
    }

    uint32_t bone_id = bone_id_lookup[bone_name];
    printf(" bone_id %d", bone_id);
    bone_parents[bone_id] = {parent_bone};
    printf(" parent_bone_id %d", parent_bone);
    printf("\n");

    if (node->mTransformation.IsIdentity())
    {
         printf("%*s{identity}\n", length * 2 + 2, "");
    }
    else
    {
         printf("%*s{"
            "{%0.2f, %0.2f, %0.2f, %0.2f}"
            ","
            "{%0.2f, %0.2f, %0.2f, %0.2f}"
            ","
            "{%0.2f, %0.2f, %0.2f, %0.2f}"
            ","
            "{%0.2f, %0.2f, %0.2f, %0.2f}"
            "\n",
            length * 2 + 2,
            "",
            node->mTransformation.a1,
            node->mTransformation.a2,
            node->mTransformation.a3,
            node->mTransformation.a4,
            node->mTransformation.b1,
            node->mTransformation.b2,
            node->mTransformation.b3,
            node->mTransformation.b4,
            node->mTransformation.c1,
            node->mTransformation.c2,
            node->mTransformation.c3,
            node->mTransformation.c4,
            node->mTransformation.d1,
            node->mTransformation.d2,
            node->mTransformation.d3,
            node->mTransformation.d4
            );
    }

    printf("%*smeshes: %d\n", length * 2 + 2, "", node->mNumMeshes);

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        process_nodes(node->mChildren[i], bone_id_lookup, bone_names, bone_parents, format, new_prefix, length + 1, bone_id);
    }
}

int
main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("usage: %s infile outfile\n", argv[0]);
        return 1;
    }
    char *inputfile = argv[1];
    char *outputfile = argv[2];

    Assimp::Importer importer;

    auto quality = aiProcessPreset_TargetRealtime_MaxQuality;
    // quality &= ~aiProcess_SplitLargeMeshes;

    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, true);
    //quality |= aiProcess_PreTransformVertices;

    // One node, one mesh
    quality |= aiProcess_OptimizeGraph;
    quality |= aiProcess_FlipUVs;
    quality &= ~aiProcess_Debone;

    // Remove everything but the mesh itself
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS | aiComponent_LIGHTS | aiComponent_CAMERAS | aiComponent_MATERIALS);
    quality |= aiProcess_RemoveComponent;
    // Remove lines and points
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    const aiScene* scene = importer.ReadFile(inputfile, quality);

    if(!scene)
    {
        // Boo.
        printf("%s\n", importer.GetErrorString());
        return 1;
    }

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        printf("Scene incomplete\n");
        return 1;
    }
    printf("Scene number of meshes: %d\n", scene->mNumMeshes);

    printf("\n");

    aiNode *root = scene->mRootNode;
    printf("Root name: %s\n", root->mName.C_Str());
    printf("Root number of children: %d\n", root->mNumChildren);
    printf("Root number of meshes: %d\n", root->mNumMeshes);

    printf("Root transformation is identity: %d\n", root->mTransformation.IsIdentity());

    printf("\n");

    if (!root->mTransformation.IsIdentity())
    {
        printf("root transformation is not identity\n");
        //return 1;
    }

    binary_format_v2 format = {};
    format.header_size = sizeof(binary_format_v2);

    uint32_t num_vertices = 0;

    std::unordered_map<std::string, uint32_t> bone_id_lookup;
    std::vector<std::string> bone_names;

    for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh *mesh = scene->mMeshes[i];
        for (uint32_t j = 0; j < mesh->mNumBones; ++j)
        {
            std::string bone_name(mesh->mBones[j]->mName.data);
            if (!bone_id_lookup.count(bone_name))
            {
                bone_id_lookup[bone_name] = (uint32_t)bone_id_lookup.size();
                // Bone name = length (1 byte) + name
                format.bone_names_size += 1 + (uint32_t)bone_name.length();
                bone_names.push_back(bone_name);
            }
        }
        format.vertices_size += sizeof(float) * 3 * mesh->mNumVertices;
        format.indices_size += sizeof(uint32_t) * 3 * mesh->mNumFaces;
        format.texcoords_size += sizeof(float) * 2 * mesh->mNumVertices;
        format.normals_size += sizeof(float) * 3 * mesh->mNumVertices;
        num_vertices += mesh->mNumVertices;
        format.num_triangles += mesh->mNumFaces;
        format.bone_ids_size += sizeof(uint32_t) * 4 * mesh->mNumVertices;
        format.bone_weights_size += sizeof(float) * 4 * mesh->mNumVertices;
    }

    std::vector<BoneParent> bone_parents;
    bone_parents.resize(bone_names.size());
    process_nodes(scene->mRootNode, bone_id_lookup, bone_names, bone_parents, format);

    uint32_t offset = format.vertices_size;
    format.texcoords_offset = offset;
    offset += format.texcoords_size;
    format.indices_offset = offset;
    offset += format.indices_size;
    format.normals_offset = offset;
    offset += format.normals_size;
    format.bone_names_offset = offset;
    offset += format.bone_names_size;
    format.bone_ids_offset = offset;
    offset += format.bone_ids_size;
    format.bone_weights_offset = offset;
    offset += format.bone_weights_size;
    format.bone_parent_offset = offset;
    offset += format.bone_parent_size;

    format.num_bones = (uint32_t)bone_parents.size();
    FILE *of = fopen(outputfile, "wb");
    char fmt[4] = { 'H', 'J', 'N', 'T' };
    size_t written = 0;
    written = fwrite(&fmt, sizeof(fmt), 1, of);
    assert(written == 1);
    uint32_t version = 2;
    written = fwrite(&version, sizeof(uint32_t), 1, of);
    assert(written == 1);
    written = fwrite(&format, sizeof(binary_format_v2), 1, of);
    assert(written == 1);

    std::vector<float> positions;
    positions.reserve(3 * num_vertices);
    std::vector<uint32_t> bone_ids;
    bone_ids.reserve(4 * num_vertices);
    std::vector<float> bone_weights;
    bone_weights.reserve(4 * num_vertices);

    bool any_bones = false;
    for (uint32_t h = 0; h < scene->mNumMeshes; ++h)
    {
        aiMesh *mesh = scene->mMeshes[h];
        std::vector<uint32_t> local_lengths;
        std::vector<BoneIds> local_bone_ids;
        std::vector<BoneWeights> local_weights;
        local_bone_ids.resize(mesh->mNumVertices);
        local_weights.resize(mesh->mNumVertices);
        local_lengths.resize(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumBones; ++i)
        {
            any_bones = true;
            aiBone *bone = mesh->mBones[i];
            std::string bone_name(bone->mName.data);
            uint32_t bone_id = bone_id_lookup[bone_name];
            for (uint32_t j = 0; j < bone->mNumWeights; ++j)
            {
                aiVertexWeight *vertex_weight = bone->mWeights + j;
                uint32_t vertex_id = vertex_weight->mVertexId;
                float weight = vertex_weight->mWeight;
                auto &&length = local_lengths[vertex_id];
                local_bone_ids[vertex_id].bones[length] = bone_id;
                local_weights[vertex_id].weights[length] = weight;
                ++length;
            }
        }
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            aiVector3D *v = mesh->mVertices + i;
            positions.push_back(v->x);
            positions.push_back(v->y);
            positions.push_back(v->z);

            bone_ids.push_back(local_bone_ids[i].bones[0]);
            bone_ids.push_back(local_bone_ids[i].bones[1]);
            bone_ids.push_back(local_bone_ids[i].bones[2]);
            bone_ids.push_back(local_bone_ids[i].bones[3]);

            bone_weights.push_back(local_weights[i].weights[0]);
            bone_weights.push_back(local_weights[i].weights[1]);
            bone_weights.push_back(local_weights[i].weights[2]);
            bone_weights.push_back(local_weights[i].weights[3]);
        }
    }

    printf("Bone parents size: %d\n", (uint32_t)bone_parents.size());
    for (uint32_t i = 0; i < bone_parents.size(); ++i)
    {
        printf("Bone[%s] has parent [%d]\n", bone_names[i].c_str(), bone_parents[i].bone_id);
    }

    fwrite(&positions[0], sizeof(float), positions.size(), of);

    if (format.texcoords_size)
    {
        std::vector<float> texcoords;
        texcoords.reserve(2 * num_vertices);
        for (uint32_t h = 0; h < scene->mNumMeshes; ++h)
        {
            aiMesh *mesh = scene->mMeshes[h];
            for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
            {
                aiVector3D *v = mesh->mTextureCoords[0] + i;
                texcoords.push_back(v->x);
                texcoords.push_back(v->y);
            }
        }
        fwrite(&texcoords[0], sizeof(float), texcoords.size(), of);
    }

    std::vector<uint32_t> indices;
    indices.reserve(3 * format.num_triangles);

    uint32_t start_index = 0;
    for (uint32_t h = 0; h < scene->mNumMeshes; ++h)
    {
        aiMesh *mesh = scene->mMeshes[h];
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
            aiFace *face = mesh->mFaces + i;
            if (face->mNumIndices != 3)
            {
                printf("Wanted only triangles, got polygon with %d indices\n", face->mNumIndices);
                return 1;
            }
            assert(face->mNumIndices == 3);
            indices.push_back(start_index + face->mIndices[0]);
            indices.push_back(start_index + face->mIndices[1]);
            indices.push_back(start_index + face->mIndices[2]);
        }
        start_index += mesh->mNumVertices;
    }

    fwrite(&indices[0], sizeof(uint32_t), indices.size(), of);

    if (format.normals_size)
    {
        std::vector<float> normals;
        normals.reserve(2 * num_vertices);
        for (uint32_t h = 0; h < scene->mNumMeshes; ++h)
        {
            aiMesh *mesh = scene->mMeshes[h];
            for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
            {
                aiVector3D *v = mesh->mNormals + i;
                normals.push_back(v->x);
                normals.push_back(v->y);
                normals.push_back(v->z);
            }
        }
        fwrite(&normals[0], sizeof(float), normals.size(), of);
    }

    if (format.bone_names_size)
    {
        for (auto it = bone_names.begin(); it != bone_names.end(); ++it)
        {
            uint8_t length = (uint8_t)it->length();
            fwrite(&length, sizeof(uint8_t), 1, of);
            fwrite(it->c_str(), sizeof(uint8_t), length, of);
            printf("bone[%s]\n", it->c_str());
        }
    }

    if (any_bones && format.bone_ids_size)
    {
        fwrite(&bone_ids[0], sizeof(float), bone_ids.size(), of);
        fwrite(&bone_weights[0], sizeof(float), bone_weights.size(), of);
        fwrite(&bone_parents[0], sizeof(BoneParent), bone_parents.size(), of);
    }

    fclose(of);
    printf("File size should be: %d bytes", (uint32_t)sizeof(binary_format_v2) + 4 + 4 + offset);
    return 0;
}
