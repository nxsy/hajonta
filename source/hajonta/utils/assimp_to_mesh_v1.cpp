#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4458)
#endif
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <string>
#include <unordered_map>
#include <vector>

struct binary_format_v3_boneless
{
    uint32_t header_size;
    uint32_t vertexformat;
    uint32_t vertices_offset;
    uint32_t vertices_size;
    uint32_t indices_offset;
    uint32_t indices_size;
    uint32_t num_triangles;
    uint32_t num_vertices;
};

struct binary_format_v3_bones
{
    uint32_t header_size;
    uint32_t vertexformat;
    uint32_t vertices_offset;
    uint32_t vertices_size;
    uint32_t indices_offset;
    uint32_t indices_size;
    uint32_t num_triangles;
    uint32_t num_vertices;

    uint32_t num_bones;
    uint32_t bone_names_offset;
    uint32_t bone_names_size;
    uint32_t bone_parent_offset;
    uint32_t bone_parent_size;
    uint32_t bone_offsets_offset;
    uint32_t bone_offsets_size;
    uint32_t bone_default_transform_offset;
    uint32_t bone_default_transform_size;

    uint32_t num_animations;
    uint32_t animation_names_offset;
    uint32_t animation_names_size;
    uint32_t animation_type_offsets_offset;
    uint32_t animation_type_offsets_size;
    uint32_t animations_offset;
    uint32_t animations_size;
};

struct
BoneAnimationHeader
{
    uint32_t version;
    float num_ticks;
    float ticks_per_second;
};

enum struct
AnimationType
{
    Unknown,
    Idle,
    Walk,
};

struct
AnimationTypeOffset
{
    uint32_t animation_type;
    uint32_t animation_offset;
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

struct v2
{
    float x;
    float y;
};

struct v3
{
    float x;
    float y;
    float z;
};

struct
Quaternion
{
    float x;
    float y;
    float z;
    float w;
};

struct
vertexformat_1
{
    v3 position;
    v3 normal;
    v2 texcoords;
};

struct
vertexformat_2
{
    v3 position;
    v3 normal;
    v2 texcoords;
    BoneIds bone_ids;
    BoneWeights bone_weights;
};

struct
vertexformat_3
{
    v3 position;
    v3 normal;
    v3 tangent;
    v2 texcoords;
};

struct
MeshBoneDescriptor
{
    v3 scale;
    v3 translate;
    Quaternion q;
};

struct
OffsetMatrix
{
    float matrix[16];
};

struct
AnimTick
{
    MeshBoneDescriptor transform;
};

struct
Animation
{
    BoneAnimationHeader header;
    std::vector<std::vector<AnimTick>> all_anim_ticks;
};

template<typename Format>
void
process_nodes(
    aiNode *node,
    std::unordered_map<std::string, uint32_t> &bone_id_lookup,
    std::vector<std::string> &bone_names,
    std::vector<BoneParent> &bone_parents,
    std::vector<MeshBoneDescriptor> &bone_default_transforms,
    Format &format,
    uint32_t length = 0,
    int32_t parent_bone = -1
)
{
    std::string bone_name(node->mName.data);
    uint32_t bone_id;
    if (!bone_id_lookup.count(bone_name))
    {
        bone_id = (uint32_t)bone_id_lookup.size();
        bone_id_lookup[bone_name] = bone_id;
        // Bone name = length (1 byte) + name
        format.bone_names_size += 1 + (uint32_t)bone_name.length();
        bone_names.push_back(bone_name);
        bone_parents.resize(bone_id + 1);
        bone_default_transforms.resize(bone_id + 1);
    }
    else
    {
        bone_id = bone_id_lookup[bone_name];
    }

    if (bone_parents[bone_id].bone_id != -1)
    {
        bone_parents[bone_id] = {parent_bone};
    }
    auto &t = node->mTransformation;
    aiVector3D scaling;
    aiVector3D position;
    aiQuaternion quat;
    t.Decompose(scaling, quat, position);

    v3 scale = {scaling.x, scaling.y, scaling.z};
    v3 translate = {position.x, position.y, position.z};
    Quaternion q = {quat.x, quat.y, quat.z, quat.w};
    MeshBoneDescriptor d = {
        scale,
        translate,
        q
    };
    bone_default_transforms[bone_id] = d;

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        process_nodes(node->mChildren[i], bone_id_lookup, bone_names, bone_parents, bone_default_transforms, format, length + 1, bone_id);
    }
}

size_t
full_fwrite(const void *ptr, size_t size, size_t count, FILE *of)
{
    size_t elements_written = fwrite(ptr, size, count, of);
    assert(elements_written == count);
    return count * size;
}

int
handle_boneless_model(const aiScene *scene, const char *outputfile)
{
    printf("Handle boneless model\n");
    binary_format_v3_boneless format = {};
    format.header_size = sizeof(binary_format_v3_boneless);

    format.vertexformat = 3;
    format.num_vertices = 0;

    for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh *mesh = scene->mMeshes[i];

        format.vertices_size += sizeof(vertexformat_3) * mesh->mNumVertices;
        format.indices_size += sizeof(uint32_t) * 3 * mesh->mNumFaces;
        format.num_vertices += mesh->mNumVertices;
        format.num_triangles += mesh->mNumFaces;
    }

    uint32_t offset = format.vertices_size;
    format.indices_offset = offset;
    offset += format.indices_size;

    std::vector<vertexformat_3> vertices;
    vertices.reserve(format.num_vertices);

    for (uint32_t h = 0; h < scene->mNumMeshes; ++h)
    {
        aiMesh *mesh = scene->mMeshes[h];
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            vertexformat_3 vf0;
            {
                aiVector3D *v = mesh->mVertices + i;
                vf0.position = { v->x, v->y, v->z};
            }
            {
                aiVector3D *v = mesh->mNormals + i;
                vf0.normal = { v->x, v->y, v->z};
            }
            {
                aiVector3D *v = mesh->mTextureCoords[0] + i;
                vf0.texcoords = { v->x, v->y };
            }
            {
                aiVector3D *v = mesh->mTangents + i;
                vf0.tangent = { v->x, v->y, v->z};
            }
            vertices.push_back(vf0);
        }
    }

    FILE *of = fopen(outputfile, "wb");
    char fmt[4] = { 'H', 'J', 'N', 'T' };
    full_fwrite(&fmt, sizeof(fmt), 1, of);
    uint32_t version = 3;
    full_fwrite(&version, sizeof(uint32_t), 1, of);
    full_fwrite(&format, sizeof(binary_format_v3_boneless), 1, of);

    size_t written = 0;
    size_t total_written = 0;
    total_written += written = full_fwrite(&vertices[0], sizeof(vertexformat_3), vertices.size(), of);
    assert(written == format.vertices_size);

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

    assert(total_written == format.indices_offset);
    total_written += written = full_fwrite(&indices[0], sizeof(uint32_t), indices.size(), of);
    assert(written == format.indices_size);

    printf("Excepted file size: %d bytes\n", (uint32_t)sizeof(binary_format_v3_boneless) + 4 + 4 + offset);
    printf("Actual file size: %d bytes\n", (uint32_t)ftell(of));
    fclose(of);

    return 0;
}

int
handle_bone_model_2(const aiScene *scene, const char *outputfile)
{
    printf("Handle bone model 2\n");
    binary_format_v3_bones format = {};
    format.header_size = sizeof(format);

    format.vertexformat = 2;
    format.num_vertices = 0;

    std::unordered_map<std::string, uint32_t> bone_id_lookup;
    std::vector<std::string> bone_names;
    std::vector<std::string> animation_names;

    std::vector<OffsetMatrix> bone_offsets;
    std::vector<MeshBoneDescriptor> bone_default_transforms;

    for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh *mesh = scene->mMeshes[i];

        format.vertices_size += sizeof(vertexformat_2) * mesh->mNumVertices;
        format.indices_size += sizeof(uint32_t) * 3 * mesh->mNumFaces;
        format.num_vertices += mesh->mNumVertices;
        format.num_triangles += mesh->mNumFaces;
        format.num_bones += mesh->mNumBones;

        for (uint32_t j = 0; j < mesh->mNumBones; ++j)
        {
            auto &bone = mesh->mBones[j];
            std::string bone_name(bone->mName.data);
            if (!bone_id_lookup.count(bone_name))
            {
                bone_id_lookup[bone_name] = (uint32_t)bone_id_lookup.size();
                // Bone name = length (1 byte) + name
                format.bone_names_size += 1 + (uint32_t)bone_name.length();
                bone_names.push_back(bone_name);

                auto mOffsetMatrix = bone->mOffsetMatrix;
                auto &t = bone->mOffsetMatrix;
                aiVector3D scaling;
                aiVector3D position;
                aiQuaternion quat;
                t.Decompose(scaling, quat, position);

                /*
                printf("%s"
                    " scale {%0.2f, %0.2f, %0.2f}"
                    " position {%0.2f, %0.2f, %0.2f}"
                    " rotation {%0.2f, %0.2f, %0.2f, %0.2f}"
                    "\n",
                    bone_name.c_str(),
                    scaling.x, scaling.y, scaling.z,
                    position.x, position.y, position.z,
                    quat.x, quat.y, quat.z, quat.w);
                    */

                printf("%s {"
                    "{%0.2f, %0.2f, %0.2f, %0.2f}"
                    ","
                    "{%0.2f, %0.2f, %0.2f, %0.2f}"
                    ","
                    "{%0.2f, %0.2f, %0.2f, %0.2f}"
                    ","
                    "{%0.2f, %0.2f, %0.2f, %0.2f}"
                    "\n",
                    bone_name.c_str(),
                    mOffsetMatrix.a1,
                    mOffsetMatrix.a2,
                    mOffsetMatrix.a3,
                    mOffsetMatrix.a4,
                    mOffsetMatrix.b1,
                    mOffsetMatrix.b2,
                    mOffsetMatrix.b3,
                    mOffsetMatrix.b4,
                    mOffsetMatrix.c1,
                    mOffsetMatrix.c2,
                    mOffsetMatrix.c3,
                    mOffsetMatrix.c4,
                    mOffsetMatrix.d1,
                    mOffsetMatrix.d2,
                    mOffsetMatrix.d3,
                    mOffsetMatrix.d4
                    );

                OffsetMatrix m = {
                    {
                        mOffsetMatrix.a1,
                        mOffsetMatrix.b1,
                        mOffsetMatrix.c1,
                        mOffsetMatrix.d1,
                        mOffsetMatrix.a2,
                        mOffsetMatrix.b2,
                        mOffsetMatrix.c2,
                        mOffsetMatrix.d2,
                        mOffsetMatrix.a3,
                        mOffsetMatrix.b3,
                        mOffsetMatrix.c3,
                        mOffsetMatrix.d3,
                        mOffsetMatrix.a4,
                        mOffsetMatrix.b4,
                        mOffsetMatrix.c4,
                        mOffsetMatrix.d4,
                    },
                };

                bone_offsets.push_back(m);
            }
        }
    }

    std::vector<BoneParent> bone_parents;
    bone_parents.resize(bone_names.size());
    bone_default_transforms.resize(bone_names.size());
    process_nodes(scene->mRootNode, bone_id_lookup, bone_names, bone_parents, bone_default_transforms, format);

    for (size_t i = bone_offsets.size(); i < bone_names.size(); ++i)
    {
        OffsetMatrix m = {
            {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1,
            },
        };

        bone_offsets.push_back(m);
    }
    bone_offsets.resize(bone_names.size());

    std::vector<Animation> hanimations;
    std::vector<AnimationTypeOffset> animation_type_offsets;

    if (scene->mNumAnimations)
    {
        format.num_animations = scene->mNumAnimations;
        hanimations.resize(scene->mNumAnimations);
        animation_type_offsets.resize(scene->mNumAnimations);
        printf("Scene has %d animations\n", scene->mNumAnimations);
        uint32_t animation_offset = 0;

        for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
        {
            uint32_t animation_size = 0;
            animation_size += sizeof(BoneAnimationHeader);
            auto &animation = scene->mAnimations[i];
            std::string animation_name(animation->mName.data);

            auto &animation_type_offset = animation_type_offsets[i];
            animation_type_offset.animation_type = (uint32_t)AnimationType::Unknown;
            animation_type_offset.animation_offset = animation_offset;

            format.animation_names_size += 1 + (uint32_t)animation_name.length();
            animation_names.push_back(animation_name);
            format.animation_type_offsets_size += sizeof(AnimationTypeOffset);

            printf("animation[%d|%s]\n", i, animation_name.c_str());
            printf("  duration %0.2f ticks at %0.2f ticks/sec\n", animation->mDuration, animation->mTicksPerSecond);

            auto &hanimation = hanimations[i];

            BoneAnimationHeader &bone_animation_header = hanimation.header;
            bone_animation_header.version = 1;
            bone_animation_header.num_ticks = (float)animation->mDuration;
            bone_animation_header.ticks_per_second = (float)animation->mTicksPerSecond;

            auto &all_anim_ticks = hanimation.all_anim_ticks;
            all_anim_ticks.resize((int32_t)animation->mDuration);
            for (int32_t j = 0; j < (int32_t)animation->mDuration; ++j)
            {
                all_anim_ticks[j].resize(bone_names.size());
            }
            printf("  bone channels: %d, mesh channels: %d\n", animation->mNumChannels, animation->mNumMeshChannels);

            for (uint32_t j = 0; j < animation->mNumChannels; ++j)
            {
                auto &anim = animation->mChannels[j];
                std::string channel_name(anim->mNodeName.data);
                assert(bone_id_lookup.count(channel_name));
                int32_t bone = bone_id_lookup[channel_name];

                printf("  Channel %s: %d position, %d rotation, %d scaling keys\n",
                    channel_name.c_str(), anim->mNumPositionKeys, anim->mNumRotationKeys, anim->mNumScalingKeys);
                /*
                printf("    Positions: ");
                printf("%0.2f", anim->mPositionKeys[0].mTime);
                */
                uint32_t position_key_number = 0;
                uint32_t scaling_key_number = 0;
                uint32_t rotation_key_number = 0;
                for (uint32_t frame = 0; frame < bone_animation_header.num_ticks; ++frame)
                {
                    auto &initial_position_key = anim->mPositionKeys[position_key_number];
                    if (frame > initial_position_key.mTime)
                    {
                        if (position_key_number < anim->mNumPositionKeys - 1)
                        {
                            ++position_key_number;
                        }
                    }
                    auto &initial_scaling_key = anim->mScalingKeys[scaling_key_number];
                    if (frame > initial_scaling_key.mTime)
                    {
                        if (scaling_key_number < anim->mNumScalingKeys - 1)
                        {
                            ++scaling_key_number;
                        }
                    }

                    auto &initial_rotation_key = anim->mRotationKeys[rotation_key_number];
                    if (frame > initial_rotation_key.mTime)
                    {
                        if (rotation_key_number < anim->mNumRotationKeys - 1)
                        {
                            ++rotation_key_number;
                        }
                    }

                    auto &position_key = anim->mPositionKeys[position_key_number];
                    auto &scaling_key = anim->mScalingKeys[scaling_key_number];
                    auto &rotation_key = anim->mRotationKeys[rotation_key_number];
                    std::vector<AnimTick> &anim_ticks = all_anim_ticks[frame];

                    anim_ticks[bone].transform.translate = {
                        position_key.mValue.x,
                        position_key.mValue.y,
                        position_key.mValue.z,
                    };

                    anim_ticks[bone].transform.scale = {
                        scaling_key.mValue.x,
                        scaling_key.mValue.y,
                        scaling_key.mValue.z,
                    };

                    anim_ticks[bone].transform.q = {
                        rotation_key.mValue.x,
                        rotation_key.mValue.y,
                        rotation_key.mValue.z,
                        rotation_key.mValue.w,
                    };
                }
            }

            animation_size += (uint32_t)(animation->mDuration * bone_names.size() * sizeof(AnimTick));
            format.animations_size += animation_size;
            animation_offset += animation_size;
        }
    }

    format.bone_offsets_size = (uint32_t)(sizeof(bone_offsets[0]) * bone_offsets.size());
    format.bone_parent_size = (uint32_t)(sizeof(BoneParent) * bone_parents.size());
    format.bone_default_transform_size = (uint32_t)(sizeof(MeshBoneDescriptor) * bone_default_transforms.size());

    uint32_t offset = format.vertices_size;
    format.indices_offset = offset;
    offset += format.indices_size;
    format.bone_names_offset = offset;
    offset += format.bone_names_size;
    format.bone_parent_offset = offset;
    offset += format.bone_parent_size;
    format.bone_offsets_offset = offset;
    offset += format.bone_offsets_size;
    format.bone_default_transform_offset = offset;
    offset += format.bone_default_transform_size;

    format.animation_names_offset = offset;
    offset += format.animation_names_size;

    format.animation_type_offsets_offset = offset;
    offset += format.animation_type_offsets_size;

    format.animations_offset = offset;
    offset += format.animations_size;

    format.num_bones = (uint32_t)bone_parents.size();

    std::vector<vertexformat_2> vertices;
    vertices.reserve(format.num_vertices);

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
            auto &&length = local_lengths[i];
            if (length == 0)
            {
                local_bone_ids[i].bones[length] = 0;
                local_weights[i].weights[length] = 1.0f;
                ++length;
            }
        }

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            vertexformat_2 vf0;
            {
                aiVector3D *v = mesh->mVertices + i;
                vf0.position = { v->x, v->y, v->z};
            }
            {
                aiVector3D *v = mesh->mNormals + i;
                vf0.normal = { v->x, v->y, v->z};
            }
            {
                aiVector3D *v = mesh->mTextureCoords[0] + i;
                vf0.texcoords = { v->x, v->y };
            }
            {
                aiVector3D *v = mesh->mTextureCoords[0] + i;
                vf0.texcoords = { v->x, v->y };
            }
            vf0.bone_ids = local_bone_ids[i];
            vf0.bone_weights = local_weights[i];

            vertices.push_back(vf0);
        }
    }

    FILE *of = fopen(outputfile, "wb");
    char fmt[4] = { 'H', 'J', 'N', 'T' };
    full_fwrite(&fmt, sizeof(fmt), 1, of);
    uint32_t version = 4;
    full_fwrite(&version, sizeof(uint32_t), 1, of);
    full_fwrite(&format, sizeof(format), 1, of);

    size_t written = 0;
    size_t total_written = 0;
    total_written += written = full_fwrite(&vertices[0], sizeof(vertexformat_2), vertices.size(), of);
    assert(written == format.vertices_size);

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

    assert(total_written == format.indices_offset);
    total_written += written = full_fwrite(&indices[0], sizeof(uint32_t), indices.size(), of);
    assert(written == format.indices_size);

    {
        assert(total_written == format.bone_names_offset);
        written = 0;
        for (auto it = bone_names.begin(); it != bone_names.end(); ++it)
        {
            uint8_t length = (uint8_t)it->length();
            written += full_fwrite(&length, sizeof(uint8_t), 1, of);
            written += full_fwrite(it->c_str(), sizeof(uint8_t), length, of);
        }
        assert(written == format.bone_names_size);
        total_written += written;
    }

    {
        printf("Bone parents:\n");
        for (uint32_t i = 0; i < format.num_bones; ++i)
        {
            printf("    %d: %d\n", i, bone_parents[i].bone_id);
        }
        assert(total_written == format.bone_parent_offset);
        printf("Bone parent offset: %d\n", format.bone_parent_offset);
        total_written += written = full_fwrite(&bone_parents[0], sizeof(BoneParent), bone_parents.size(), of);
        assert(written == format.bone_parent_size);

        assert(total_written == format.bone_offsets_offset);
        total_written += written = full_fwrite(&bone_offsets[0], sizeof(OffsetMatrix), bone_offsets.size(), of);
        assert(written == format.bone_offsets_size);

        assert(total_written == format.bone_default_transform_offset);
        total_written += written = full_fwrite(&bone_default_transforms[0], sizeof(MeshBoneDescriptor), bone_default_transforms.size(), of);
        assert(written == format.bone_default_transform_size);
    }


    {
        assert(total_written == format.animation_names_offset);
        written = 0;
        for (auto it = animation_names.begin(); it != animation_names.end(); ++it)
        {
            uint8_t length = (uint8_t)it->length();
            written += full_fwrite(&length, sizeof(uint8_t), 1, of);
            written += full_fwrite(it->c_str(), sizeof(uint8_t), length, of);
        }
        assert(written == format.animation_names_size);
        total_written += written;
    }

    assert(total_written == format.animation_type_offsets_offset);
    printf("Animation type offsets offset: %zd\n", total_written);
    {
        total_written += written = full_fwrite(&animation_type_offsets[0], sizeof(AnimationTypeOffset), format.num_animations, of);
    }

    assert(total_written == format.animations_offset);
    size_t anim_written = 0;
    for (uint32_t j = 0; j < format.num_animations; ++j)
    {
        auto &hanimation = hanimations[j];
        auto &bone_animation_header = hanimation.header;
        auto &all_anim_ticks = hanimation.all_anim_ticks;
        auto &ato = animation_type_offsets[j];

        printf("Animation type offset: %s, %d, %d\n", animation_names[j].c_str(), ato.animation_type, ato.animation_offset);
        anim_written += written = full_fwrite(&bone_animation_header, sizeof(BoneAnimationHeader), 1, of);
        for (uint32_t i = 0; i < (uint32_t)bone_animation_header.num_ticks; ++i)
        {
            std::vector<AnimTick> &anim_ticks = all_anim_ticks[i];
            anim_written += written = full_fwrite(&anim_ticks[0], sizeof(AnimTick), format.num_bones, of);
        }
    }
    total_written += anim_written;
    assert(anim_written == format.animations_size);
    assert(total_written == offset);

    printf("Excepted file size: %d bytes\n", (uint32_t)sizeof(format) + 4 + 4 + offset);
    printf("Actual file size: %d bytes\n", (uint32_t)ftell(of));
    fclose(of);

    return 0;
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

    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, false);
    //quality |= aiProcess_PreTransformVertices;
    //

    // One node, one mesh
    quality |= aiProcess_OptimizeGraph;
    quality |= aiProcess_OptimizeMeshes;  // In MaxQuality already
    quality |= aiProcess_FlipUVs;
    //quality |= aiProcess_Debone;  // In MaxQuality already, is lossless apparently
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

    bool has_bones = false;
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh *mesh = scene->mMeshes[i];
        if (mesh->mNumBones)
        {
            has_bones = true;
            break;
        }
    }
    if (!has_bones)
    {
        quality &= ~aiProcess_OptimizeGraph;
        quality |= aiProcess_PreTransformVertices;
        scene = importer.ReadFile(inputfile, quality);
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

    if (has_bones)
    {
        return handle_bone_model_2(scene, outputfile);
    }
    else
    {
        return handle_boneless_model(scene, outputfile);
    }
    return 0;
}

