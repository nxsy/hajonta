/*
 * Currently, finds all *.asset files in the current working directory and
 * builds an asset pack called "assets.pack" from that information.
 *
 * Currently it just builds a sparse pack with the content provided by a
 * filename.
 *
 * Currently, it has to load the files to determine their image dimensions.
 *
 * In future, we should have a .assetin which just has the filename and other
 * static information, which creates the .asset file with the necessary asset
 * information, only if the .assetin (or source files mentioned in it) change,
 * and this .asset file will be used (with no image loading) to generate the
 * pack file.
 *
 * The .assetin format might, for example, describe how to generate the in-game
 * format (say, .hjm for mesh) from a non-in-game source format (say, .fbx),
 * and rebuild the in-game format file if the source format file changes.
 *
 */

/*
 * NOTE(nbm): Why is this using Window's APIs for directory traversal and file
 * opening? Mostly, the roughly-formed thought was that in one mode, this would
 * watch for and receive directory update events, and we would then rebuild as
 * necessary.  It might even sit on the task bar or otherwise run in the
 * background.
 *
 * More likely this'll never be slow enough, and changes won't happen
 * frequently enough, and nobody else would be creating assets, that just
 * running the command in a command prompt won't be good enough.
 */

#include <assert.h>
#include <stdint.h>
#include <windows.h>

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "hajonta/math.h"
#include "hajonta/assets.h"
#include "hajonta/algo/fnv1a.cpp"

#define STB_IMAGE_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(push, 4)
#pragma warning(disable: 4365 4312 4456 4457)
#endif
#include "hajonta/thirdparty/stb_image.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct
RuntimeData
{
    std::vector<Asset> assets;
    std::vector<AssetPiece> asset_pieces;
    std::vector<std::string> filenames;
    uint32_t filenames_size;
};

void
add_asset(RuntimeData *rtdata, std::string path, std::string relative_path)
{
    std::string asset_name = relative_path.substr(0, relative_path.size() - 6);
    printf("Asset name is %s\n", asset_name.c_str());

    std::ifstream file(path);
    std::string type;
    getline(file, type);

    std::string filename;
    getline(file, filename);

    std::string relative_filename = relative_path.substr(0, relative_path.rfind("\\") + 1) + filename;
    filename = path.substr(0, path.rfind("\\") + 1) + filename;

    printf("Type is %s\n", type.c_str());
    printf("Filename is %s\n", filename.c_str());
    printf("Relative filename is %s\n", relative_filename.c_str());

    printf("Opening file %s\n", filename.c_str());
    HANDLE handle = CreateFile(
        filename.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if (handle == INVALID_HANDLE_VALUE)
    {
        DWORD error_code = GetLastError();
        printf("Error code: %d", error_code);
        assert(handle != INVALID_HANDLE_VALUE);
    }

    LARGE_INTEGER _size;
    GetFileSizeEx(handle, &_size);
    printf("GetFileSize returns size of %lld\n", _size.QuadPart);

    unsigned char *buffer = (unsigned char *)malloc(_size.QuadPart);
    DWORD _bytes_read;
    bool loaded = ReadFile(
        handle,
        buffer,
        (DWORD)_size.QuadPart,
        &_bytes_read,
        0);
    assert(_size.QuadPart == _bytes_read);
    assert(loaded);

    if (type == "texture")
    {
        Asset a;
        a.asset_type = AssetType::Texture;
        a.asset_sub_type = AssetSubType::Diffuse;
        a.last_modified = 0;
        a.num_asset_pieces = 1;
        a.asset_piece_id = (uint32_t)rtdata->asset_pieces.size();
        printf("Asset name is %s\n", asset_name.c_str());
        uint32_t asset_id = fnv1a_32((uint8_t *)asset_name.c_str(), (uint32_t)asset_name.size());
        a.asset_id = asset_id;
        rtdata->assets.push_back(a);

        int x, y, channels;
        unsigned char *buffer2 = stbi_load_from_memory(buffer, (int)_size.QuadPart, &x, &y, &channels, 0);
        stbi_image_free(buffer2);
        AssetPiece p;
        p.offset = rtdata->filenames_size;
        rtdata->filenames.push_back(relative_filename);
        rtdata->filenames_size += (uint32_t)relative_filename.size() + 1;
        p.size = (uint32_t)_size.QuadPart;
        printf("Asset piece size is %ld\n", p.size);
        p.metadata.texture.container_format = AssetContainerFormat::Plain;
        p.metadata.texture.texture_format = AssetTextureFormat::CompressedImage;
        p.metadata.texture.dimension = {x, y, 0};
        p.metadata.texture.bpp = 32;
        p.metadata.texture.num_mipmaps = 1;
        rtdata->asset_pieces.push_back(p);
    }
    free(buffer);
}

struct
PathPair
{
    std::string full_path;
    std::string relative_path;
};

void
traverse(RuntimeData *rtdata, const char *path)
{
    std::vector<PathPair> paths_to_traverse =
    {
        {path, ""},
    };

    while (paths_to_traverse.size())
    {
        PathPair p = paths_to_traverse.back();
        std::string this_path = p.full_path;
        paths_to_traverse.pop_back();

        std::string search_path = this_path + "\\*";
        WIN32_FIND_DATA data;
        HANDLE handle = FindFirstFileEx(
            search_path.c_str(),
            FindExInfoBasic,
            &data,
            FindExSearchNameMatch,
            0,
            FIND_FIRST_EX_LARGE_FETCH
        );

        if (handle == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        do
        {
            if (data.cFileName[0] == '.')
            {
                //printf("Starts with .: %s\n", data.cFileName);
                continue;
            }
            std::string full_path = this_path + "\\" + data.cFileName;
            std::string relative_path;
            if (p.relative_path.size())
            {
                relative_path = p.relative_path + "\\" + data.cFileName;
            }
            else
            {
                relative_path = data.cFileName;
            }
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
            {
                PathPair newp =
                {
                    full_path,
                    relative_path
                };
                paths_to_traverse.push_back(newp);
                continue;
            }
            size_t last_dot = full_path.rfind(".asset");
            if (last_dot == -1)
            {
                continue;
            }
            std::string extension = full_path.substr(last_dot);
            if (extension != ".asset")
            {
                continue;
            }
            add_asset(rtdata, full_path, relative_path);
        } while (FindNextFile(handle, &data));
    }
}

size_t
full_fwrite(const void *ptr, size_t size, size_t count, FILE *of)
{
    size_t elements_written = fwrite(ptr, size, count, of);
    assert(elements_written == count);
    return count * size;
}

void
write_file(RuntimeData *rtdata, std::string outputfile)
{
    AssetFile_0 af = {};
    af.header_size = sizeof(af);
    printf("Header size: %d\n", af.header_size);
    af.piece_storage_type = AssetPieceStorageType::Files;
    af.asset_count = (uint32_t)rtdata->assets.size();
    printf("Asset count: %d\n", af.asset_count);
    af.asset_piece_count = (uint32_t)rtdata->asset_pieces.size();
    printf("Piece count: %d\n", af.asset_piece_count);

    // TODO(nbm): No, I don't know what I'm doing.
    af.asset_hash_size = 1;
    while (af.asset_count + 10 > af.asset_hash_size)
    {
        af.asset_hash_size *= 2 + 1;
    }
    printf("Asset Hash entries: %d\n", af.asset_hash_size);

    uint32_t offset = 0;
    af.asset_offset = offset;
    af.asset_size = sizeof(Asset) * af.asset_count;
    printf("Asset offset: %d\n", af.asset_offset);
    printf("Asset size: %d\n", af.asset_size);
    offset += af.asset_size;

    af.piece_offset = offset;
    af.piece_size = sizeof(AssetPiece) * af.asset_piece_count;
    printf("Piece offset: %d\n", af.piece_offset);
    printf("Piece size: %d\n", af.piece_size);
    offset += af.piece_size;

    af.hash_offset = offset;
    af.hash_size = af.asset_hash_size * sizeof(uint32_t);
    printf("Hash offset: %d\n", af.hash_offset);
    printf("Hash size: %d\n", af.hash_size);
    offset += af.hash_size;

    af.filename_offset = offset;
    af.filename_size = rtdata->filenames_size;
    printf("Filename offset: %d\n", af.filename_offset);
    printf("Filename size: %d\n", af.filename_size);


    FILE *of = fopen(outputfile.c_str(), "wb");
    char fmt[4] = { 'H', 'J', 'N', 'P' };
    full_fwrite(&fmt, sizeof(fmt), 1, of);
    uint32_t version = 0;
    full_fwrite(&version, sizeof(uint32_t), 1, of);
    full_fwrite(&af, sizeof(AssetFile_0), 1, of);

    size_t written = 0;
    size_t total_written = 0;

    assert(total_written == af.asset_offset);
    total_written += written = full_fwrite(&rtdata->assets[0], sizeof(Asset), rtdata->assets.size(), of);
    assert(written == af.asset_size);

    assert(total_written == af.piece_offset);
    total_written += written = full_fwrite(&rtdata->asset_pieces[0], sizeof(AssetPiece), rtdata->asset_pieces.size(), of);
    assert(written == af.piece_size);

    std::vector<uint32_t> hash(af.asset_hash_size);
    for (uint32_t i = 0; i < af.asset_count; ++i)
    {
        Asset &asset = rtdata->assets[i];
        printf("Asset %d with asset_id %d\n", i, asset.asset_id);
        hash[asset.asset_id % af.asset_hash_size] = i + 1;
    }

    assert(total_written == af.hash_offset);
    total_written += written = full_fwrite(&hash[0], sizeof(uint32_t), hash.size(), of);
    assert(written == af.hash_size);

    assert(total_written == af.filename_offset);
    char *filenames = (char *)malloc(af.filename_size);
    uint32_t filenames_offset = 0;
    for (uint32_t i = 0; i < rtdata->filenames.size(); ++i)
    {
        strcpy(filenames + filenames_offset, rtdata->filenames[i].c_str()); //, filenames[i].size());
        filenames_offset += (uint32_t)rtdata->filenames[i].size() + 1;
    }
    assert(filenames_offset == af.filename_size);
    total_written += written = full_fwrite(filenames, af.filename_size, 1, of);
    assert(written == af.filename_size);
}

int
main(int argc, char **argv)
{
    AssetFile_0 asset_file = {};
    asset_file.header_size = sizeof(AssetFile_0);
    asset_file.piece_storage_type = AssetPieceStorageType::Files;

    char path[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, path);

    RuntimeData rtdata = {};

    std::string outputfilename = std::string(path) + "\\assets.pack";
    traverse(&rtdata, path);
    write_file(&rtdata, outputfilename);
}
