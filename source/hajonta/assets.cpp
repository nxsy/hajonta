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
AssetSubType
{
    Diffuse,
    Normal,
    Specular,
    Cubemap,
    MeshShader,
    ShaderFilter,
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

Asset *
find_asset_in_pack(AssetPack *pack, uint32_t asset_id)
{
    Asset *result = 0;
    uint32_t hash_index = asset_id % pack->asset_hash_size;
    uint32_t asset_index_in_pack = pack->asset_hash[hash_index];
    if (!asset_index_in_pack)
    {
        return result;
    }
    asset_index_in_pack--;
    result = pack->assets + asset_index_in_pack;
    return result;
}

AssetPiece *
choose_asset_piece_from_asset(AssetPack *pack, Asset *asset)
{
    AssetPiece *result = 0;
    if (!asset)
    {
        return result;
    }
    // TODO(nbm): Don't just choose the first asset piece, actually apply some
    // intelligence here.  This is probably a platform call.
    result = pack->asset_pieces + asset->asset_piece_id;
    return result;
}

struct
LoadedTextureAsset
{
    bool initialized;
    int32_t texaddress_index;
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

LoadedAsset *
get_asset_from_hash(AssetHash *hash, uint32_t asset_id)
{
    uint32_t asset_hash = asset_id % hash->asset_hash_size;
    AssetHashEntry *possibility = hash->hash[asset_hash];

    while (possibility && possibility->loaded_asset.asset)
    {
        if (possibility->loaded_asset.asset->asset_id == asset_id)
        {
            return &possibility->loaded_asset;
        }
    }

    return 0;
}

LoadedAsset *
add_asset_to_hash(AssetHash *hash, AssetPack *pack, Asset *asset, AssetPiece *asset_piece)
{
    LoadedAsset *result = get_asset_from_hash(hash, asset->asset_id);

    if (result)
    {
        return result;
    }

    hassert(hash->free_entries);
    AssetHashEntry *entry = hash->free_entries;
    AssetHashEntry *next = hash->free_entries->next;

    result = &entry->loaded_asset;
    result->pack = pack;
    result->asset = asset;
    result->asset_piece = asset_piece;

    uint32_t asset_hash = asset->asset_id % hash->asset_hash_size;
    AssetHashEntry *possibility = hash->hash[asset_hash];
    if (!possibility)
    {
        hash->hash[asset_hash] = entry;
    }
    else
    {
        while (possibility->next)
        {
            possibility = possibility->next;
        }
        possibility->next = entry;
    }
    hash->free_entries = next;

    return result;
}

void
asset_hash_add_list(AssetHash *hash)
{
    AssetHashList *list = (AssetHashList *)PushStruct("asset_hash_list", hash->arena, AssetHashList);
    list->next = hash->list;
    hash->list = list;
    list->entries[0].next = hash->free_entries;
    for (uint32_t i = 1; i < harray_count(list->entries); ++i)
    {
        list->entries[i].next = list->entries + i - 1;
    }
    hash->free_entries = list->entries + harray_count(list->entries) - 1;
    return;
}

void
asset_hash_init(AssetHash* hash, MemoryArena *arena, uint32_t max_assets, uint32_t hash_size)
{
    hash->arena = arena;
    hash->hash = (AssetHashEntry **)PushSize("asset_hash", arena, sizeof(AssetHashEntry *) * hash_size);
    hash->asset_hash_size = hash_size;
    hash->free_entries = 0;
    hash->list = 0;
    hash->num_assets = 0;
    hash->max_assets = max_assets;

    asset_hash_add_list(hash);
}

struct
AssetManagementState
{
    MemoryArena *arena;
    AssetHash asset_hash;
    AssetPackPointerList packs;
};


void
asset_management_state_init(AssetManagementState *state, MemoryArena *arena, uint32_t max_assets, uint32_t hash_size)
{

    /*
    state->max_assets = max_assets;
    state->assets = (LoadedAsset *)PushSize("loaded_assets", arena, sizeof(LoadedAsset) * state->asset_state.max_assets);
    state->asset_hash_size = 256;
    state->asset_hash = (uint32_t *)PushSize("loaded_asset_hash", arena, sizeof(uint32_t) * state->asset_state.asset_hash_size);
    */
    state->arena = arena;
    asset_hash_init(&state->asset_hash, arena, max_assets, hash_size);
}

void
add_pack_to_asset_management_state(AssetManagementState *state, AssetPack *pack)
{
    state->packs.packs[state->packs.num_packs++] = pack;
}

LoadedAsset *
load_asset(AssetManagementState *state, uint32_t asset_id, int32_t asset_piece_id = -1)
{
    LoadedAsset *result = get_asset_from_hash(&state->asset_hash, asset_id);
    if (result)
    {
        return result;
    }

    AssetPackPointerList *list = &state->packs;
    while (list)
    {
        bool found = false;
        for (uint32_t i = 0; i < list->num_packs; ++i)
        {
            AssetPack *pack = list->packs[i];
            Asset *asset = find_asset_in_pack(pack, asset_id);
            if (!asset)
            {
                continue;
            }
            AssetPiece *asset_piece = choose_asset_piece_from_asset(pack, asset);

            if (!asset_piece)
            {
                continue;
            }

            result =  add_asset_to_hash(&state->asset_hash, pack, asset, asset_piece);
            found = true;
            break;
        }
        if (found)
        {
            break;
        }
        list = list->next;
    }
    return result;
}


