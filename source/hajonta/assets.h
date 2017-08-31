enum struct
AssetType
{
    Texture,
    Mesh,
    Material,
    Prefab,
    Audio,
    Image,
    Shader,
};

enum struct
TextureAssetSubType
{
    Diffuse,
    Normal,
    Specular,
    //Cubemap,
};

enum struct
MeshAssetSubType
{
    Unknown,
};

/*
    MeshShader,
    ShaderFilter,
*/

union
AssetSubType
{
    TextureAssetSubType texture;
    MeshAssetSubType mesh;
};

enum struct
AssetTextureFormat
{
    /*
     * Something like a PNG that probably needs uncompressing
     * before going to the graphics card
     */
    CompressedImage,

    /*
     * Something like an uncompressed bitmap or TGA that can
     * be copied straight to the graphics card
     */
    Pixmap,

    /* Compressed formats that can be sent directly to
     * graphics cards that support the format
     */
    // DXT,  // Can we assume all DXT1-5 supported by cards if
             // any is?
    // ETC2,
    // PVRTC,
    // ASTC
};

enum struct
AssetContainerFormat
{
    /*
     * A plain image file, like a PNG.  Only one face, no
     * mipmaps.
     */
    Plain,

    /*
     * A DDS file contains up to six faces, multiple mipmaps,
     * generally compressed with DXT.
     */
    // DDS,

    /*
     * A KTX file contains up to six faces and multiple
     * mipmaps, supporting any(?) OpenGL-recognized formats
     * and internal formats.
     */
    // KTX,
};

struct
Asset
{
    uint32_t asset_id;
    AssetType asset_type;
    AssetSubType asset_sub_type;

    // Useful when determining whether to rebuild this asset
    // by comparing to its source files.
    uint32_t last_modified;

    uint32_t num_asset_pieces;

    // When packaged, where the first asset_piece is stored.
    uint32_t asset_piece_id;
};

/*
 * This is metadata that might help the asset system decide which of the pieces
 * is the most appropriate to load on this particular device.
 *
 * Preferences might be related to the capabilities of the device - like
 * whether the device supports DXT, ETC, PVRTC, and so forth.  But if the
 * device type doesn't support a particular type (say, iPhone not supporting
 * FOO), those asset pieces would just not be in the package in the first
 * place.
 *
 * Or they might just have to do with the horsepower of the device - like using
 * the 1024x1024 image vs. the 4096x4096.  But with container formats which
 * support mipmaps, we could probably just load the subset of mipmaps that the
 * device can handle.
 */
struct
AssetPieceTextureMetadata
{
    AssetContainerFormat container_format;
    AssetTextureFormat texture_format;
    v3i dimension;  // Up to 3 dimensions
    uint32_t bpp;
    uint32_t num_mipmaps;
};

/*
 * This is metadata that might help the asset system decide which of the pieces
 * is the most appropriate to load on this particular device.
 *
 * Preferences might be related to the capabilities of the device - like
 * whether the device supports DXT, ETC, PVRTC, and so forth.  But if the
 * device type doesn't support a particular type (say, iPhone not supporting
 * FOO), those asset pieces would just not be in the package in the first
 * place.
 *
 * Or they might just have to do with the horsepower of the device - like using
 * the 1024x1024 image vs. the 4096x4096.  But with container formats which
 * support mipmaps, we could probably just load the subset of mipmaps that the
 * device can handle.
 */

struct
AssetPieceMeshMetadata
{
    uint32_t num_vertices;
    // rectangle3 bounding_cube;
};

struct
AssetPieceMetadata
{
    union
    {
        AssetPieceTextureMetadata texture;
        AssetPieceMeshMetadata mesh;
    };
};

struct
AssetPiece
{
    // Where in the asset piece binary section the asset piece's content lives
    uint32_t offset;
    uint32_t size;

    AssetPieceMetadata metadata;
};

enum struct
AssetPieceStorageType
{
    Embedded,
    Files,
};

struct
AssetPack
{
    AssetPieceStorageType piece_storage_type;
    Asset *assets;
    uint32_t asset_count;

    uint32_t *asset_hash;
    uint32_t asset_hash_size;

    AssetPiece *asset_pieces;
    uint32_t asset_piece_count;

    /*
     * If piece_storage_type is Embedded, then pieces are fetched from the
     * package file at piece.offset.  If it is Files, the filename is fetched
     * from filenames[offset], and the piece content is the full file contents.
     */
    char *filenames;
};

struct
LoadedTextureAsset
{
    bool initialized;
    int32_t texaddress_index;
};

struct
LoadedMeshAsset
{
    bool initialized;
    Mesh *mesh;
    int32_t load_state;
    V3Bones *v3bones;
};

struct
LoadedAsset
{
    AssetPack *pack;
    Asset *asset;
    AssetPiece *asset_piece;

    union
    {
        LoadedTextureAsset texture;
        LoadedMeshAsset mesh;
    };
};

struct
AssetPackPointerList
{
    uint32_t num_packs;
    AssetPack *packs[16];
    AssetPackPointerList *next;
};

struct
AssetHashEntry
{
    LoadedAsset loaded_asset;
    AssetHashEntry *next;
};

struct
AssetHashList
{
    AssetHashEntry entries[10];
    AssetHashList *next;
};

struct MemoryArena;

struct
AssetHash
{
    // If we need more memory
    MemoryArena *arena;

    // Points to a list pointers to AssetHashEntry
    AssetHashEntry **hash;
    // Size of the hash currently.  Will probably grow later
    uint32_t asset_hash_size;

    // Where to get next pointers in case of collision
    AssetHashEntry *free_entries;
    // We get new AssetHashEntry space in bunches
    AssetHashList *list;

    uint32_t num_assets;
    uint32_t max_assets;
};

struct
AssetManagementState
{
    MemoryArena *arena;
    AssetHash asset_hash;
    AssetPackPointerList packs;
};

struct
AssetFile_0
{
    uint32_t header_size;
    AssetPieceStorageType piece_storage_type;
    uint32_t asset_count;
    uint32_t asset_piece_count;
    uint32_t asset_hash_size;

    uint32_t asset_offset;
    uint32_t asset_size;
    uint32_t piece_offset;
    uint32_t piece_size;
    uint32_t hash_offset;
    uint32_t hash_size;
    uint32_t filename_offset;
    uint32_t filename_size;
};

