#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <vector>

struct binary_format_v1
{
    uint32_t version;

    uint32_t vertices_offset;
    uint32_t vertices_size;
    uint32_t normals_offset;
    uint32_t normals_size;
    uint32_t texcoords_offset;
    uint32_t texcoords_size;
    uint32_t indices_offset;
    uint32_t indices_size;
    uint32_t num_triangles;
};

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
    quality |= aiProcess_PreTransformVertices;

    // One node, one mesh
    quality |= aiProcess_OptimizeGraph;
    quality |= aiProcess_FlipUVs;

    // Remove everything but the mesh itself
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS | aiComponent_BONEWEIGHTS | aiComponent_ANIMATIONS | aiComponent_LIGHTS | aiComponent_CAMERAS | aiComponent_MATERIALS);
    quality |= aiProcess_RemoveComponent;

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

    /*
    printf("Scene number of animations: %d\n", scene->mNumAnimations);
    printf("Scene number of meshes: %d\n", scene->mNumMeshes);

    printf("\n");

    aiNode *root = scene->mRootNode;
    printf("Root name: %s\n", root->mName.C_Str());
    printf("Root number of children: %d\n", root->mNumChildren);
    printf("Root number of meshes: %d\n", root->mNumMeshes);

    printf("Root transformation is identity: %d\n", root->mTransformation.IsIdentity());

    printf("\n");

    for (uint32_t i = 0; i < root->mNumChildren; ++i)
    {
        aiNode *child = root->mChildren[i];;
        printf("child[%d] name: %s\n", i, child->mName.C_Str());
        printf("child[%d] number of children: %d\n", i, child->mNumChildren);
        printf("child[%d] number of meshes: %d\n", i, child->mNumMeshes);
        printf("child[%d] transformation is identity: %d\n", i, child->mTransformation.IsIdentity());

        printf("\n");
    }


    printf("Yatta!");
    */

    aiNode *root = scene->mRootNode;
    assert(root->mNumChildren == 0);
    assert(root->mNumMeshes == 1);
    assert(root->mTransformation.IsIdentity());

    aiMesh *mesh = scene->mMeshes[0];

    assert(mesh->HasPositions());

    printf("mesh vertices: %d\n", mesh->mNumVertices);
    printf("mesh faces: %d\n", mesh->mNumFaces);

    binary_format_v1 format = {};
    format.version = 1;
    format.vertices_size = sizeof(float) * 3 * mesh->mNumVertices;
    format.indices_size = sizeof(uint32_t) * 3 * mesh->mNumFaces;
    if (mesh->HasTextureCoords(0))
    {
        format.texcoords_size = sizeof(float) * 2 * mesh->mNumVertices;
    }
    format.num_triangles = mesh->mNumFaces;

    format.texcoords_offset = format.vertices_size;
    format.indices_offset = format.vertices_size + format.texcoords_size;

    FILE *of = fopen(outputfile, "wb");
    char fmt[4] = { 'H', 'J', 'N', 'T' };
    size_t written = 0;
    written = fwrite(&fmt, sizeof(fmt), 1, of);
    assert(written == 1);
    written = fwrite(&format, sizeof(binary_format_v1), 1, of);
    assert(written == 1);

    std::vector<float> positions;
    positions.reserve(3 * mesh->mNumVertices);
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        aiVector3D *v = mesh->mVertices + i;
        positions.push_back(v->x);
        positions.push_back(v->y);
        positions.push_back(v->z);
    }

    fwrite(&positions[0], sizeof(float), positions.size(), of);

    if (format.texcoords_size)
    {
        std::vector<float> texcoords;
        texcoords.reserve(2 * mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            aiVector3D *v = mesh->mTextureCoords[0] + i;
            texcoords.push_back(v->x);
            texcoords.push_back(v->y);
        }
        fwrite(&texcoords[0], sizeof(float), texcoords.size(), of);
    }

    std::vector<uint32_t> indices;
    indices.reserve(3 * mesh->mNumFaces);

    for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
        aiFace *face = mesh->mFaces + i;
        assert(face->mNumIndices == 3);
        indices.push_back(face->mIndices[0]);
        indices.push_back(face->mIndices[1]);
        indices.push_back(face->mIndices[2]);
    }

    fwrite(&indices[0], sizeof(uint32_t), indices.size(), of);

    fclose(of);
    return 0;
}
