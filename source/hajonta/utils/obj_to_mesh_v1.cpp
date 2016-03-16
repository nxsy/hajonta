#include <stdio.h>
#include <iostream>
#define TINYOBJLOADER_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(disable: 4815)
#pragma warning(push, 4)
#pragma warning(disable: 4706)
#endif
#include "tiny_obj_loader.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

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
    assert(argc >= 2);
    char *inputfile = argv[1];
    char *outputfile = argv[2];
    std::string inputfilepath(inputfile);
    inputfilepath.resize(inputfilepath.rfind("\\") + 1);

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;
    bool ret = tinyobj::LoadObj(shapes, materials, err, inputfile, inputfilepath.c_str(), false);

    if (!err.empty()) { // `err` may contain warning message.
      std::cerr << err << std::endl;
    }

    if (!ret) {
      exit(1);
    }

    binary_format_v1 format = {};
    format.version = 1;

    for (size_t i = 0; i < shapes.size(); i++) {
        auto &&mesh = shapes[i].mesh;

        format.indices_size += (uint32_t)mesh.indices.size() * sizeof(mesh.indices[0]);
        format.vertices_size += sizeof(float) * (uint32_t)mesh.positions.size();
        format.normals_size += sizeof(float) * (uint32_t)mesh.normals.size();
        format.texcoords_size += sizeof(float) * (uint32_t)mesh.texcoords.size();

        for (size_t n = 0; n < shapes[i].mesh.num_vertices.size(); n++) {
            int ngon = shapes[i].mesh.num_vertices[n];
            assert(ngon == 3);
            format.num_triangles++;
        }
    }

    format.texcoords_offset = format.vertices_size;
    format.indices_offset = format.vertices_size + format.texcoords_size;

    FILE *of = fopen(outputfile, "wb");
    char fmt[4] = { 'H', 'J', 'N', 'T' };
    size_t written = 0;
    written = fwrite(&fmt, sizeof(fmt), 1, of);
    assert(written == 1);
    written = fwrite(&format, sizeof(binary_format_v1), 1, of);
    assert(written == 1);

    for (size_t i = 0; i < shapes.size(); i++) {
        auto &&mesh = shapes[i].mesh;
        fwrite(&mesh.positions[0], sizeof(float), mesh.positions.size(), of);
        /*
        uint32_t num_vertices = (uint32_t)mesh.positions.size() / 3;

        for (uint32_t v = 0; v < num_vertices; ++v)
        {
            float x = mesh.positions[v*3+0];
            float y = mesh.positions[v*3+1];
            float z = mesh.positions[v*3+2];
            printf("Vertex[%d] = %f, %f, %f\n",
                v,
                x, y, z);
            written = fwrite(&x, 4, 1, of);
            assert(written == 1);
            written = fwrite(&y, 4, 1, of);
            assert(written == 1);
            written = fwrite(&z, 4, 1, of);
            assert(written == 1);
        }
        */
    }

    if (format.texcoords_size)
    {
        for (size_t i = 0; i < shapes.size(); i++) {
            auto &&mesh = shapes[i].mesh;
            uint32_t num_texcoords = (uint32_t)mesh.texcoords.size() / 2;

            for (uint32_t ii = 0; ii < num_texcoords; ++ii)
            {
                float s = mesh.texcoords[ii*2+0];
                float t = 1.0f - mesh.texcoords[ii*2+1];
                written = fwrite(&s, sizeof(float), 1, of);
                assert(written == 1);
                written = fwrite(&t, sizeof(float), 1, of);
                assert(written == 1);
            }
        }
    }

    for (size_t i = 0; i < shapes.size(); i++) {
        auto &&mesh = shapes[i].mesh;
        fwrite(&mesh.indices[0], sizeof(float), mesh.indices.size(), of);
        /*
        for (size_t face = 0; face < shapes[i].mesh.indices.size() / 3; face++) {
            fwrite(&shapes[i].mesh.indices[3*face+0], sizeof(int32_t), 3, of);
            printf("indices[%zd] = %d, %d, %d\n",
                    face,
                    mesh.indices[face*3+0],
                    mesh.indices[face*3+1],
                    mesh.indices[face*3+2]);
        }
        */
    }
    fclose(of);
}
