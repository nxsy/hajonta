#pragma once

#include "hajonta/math.h"

struct
tile_priority_queue_entry
{
    float score;
    v2i tile_position;
};

struct
tile_priority_queue
{
    uint32_t max_entries;
    uint32_t num_entries;
    tile_priority_queue_entry *entries;
};

struct
priority_queue_debug
{
    bool show;
    float value;
    tile_priority_queue_entry entries[20];
    tile_priority_queue queue;
    uint32_t selected;

    uint32_t num_sorted;
    float sorted[20];
};

void queue_add(tile_priority_queue *queue, tile_priority_queue_entry entry);
tile_priority_queue_entry queue_pop(tile_priority_queue *queue);
void queue_init(tile_priority_queue *queue, uint32_t max_entries, tile_priority_queue_entry *entries);
