#include "hajonta/assets.h"

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
        possibility = possibility->next;
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


