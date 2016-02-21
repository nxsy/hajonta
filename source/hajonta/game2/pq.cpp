#pragma once
#include "hajonta/game2/pq.h"

void
queue_clear(tile_priority_queue *queue)
{
    queue->num_entries = 0;
}

static void
_queue_fix_up(tile_priority_queue *queue, uint32_t entry_position)
{
    while (entry_position != 0)
    {
        auto entry = queue->entries[entry_position];
        uint32_t parent_position = (entry_position - 1) / 2;
        auto parent = queue->entries[parent_position];
        if (entry.score >= parent.score)
        {
            break;
        }

        queue->entries[entry_position] = queue->entries[parent_position];
        uint32_t sibling_position = 2 * parent_position + 1;
        if (sibling_position == entry_position)
        {
            ++sibling_position;
        }
        if (sibling_position < queue->num_entries)
        {
            auto sibling = queue->entries[sibling_position];

            if (entry.score >= sibling.score)
            {
                queue->entries[parent_position] = queue->entries[sibling_position];
                queue->entries[sibling_position] = entry;
                break;
            }
        }
        queue->entries[parent_position] = entry;
        entry_position = parent_position;
    }
}

static void
_queue_fix_down(tile_priority_queue *queue, uint32_t entry_position)
{
    uint32_t parent_position = 0;
    while (true)
    {
        auto parent = queue->entries[parent_position];
        uint32_t left_child_position = 2 * parent_position + 1;
        uint32_t right_child_position = 2 * parent_position + 2;

        if (left_child_position >= queue->num_entries)
        {
            break;
        }
        auto left_child = queue->entries[left_child_position];
        if (right_child_position >= queue->num_entries)
        {
            if (left_child.score < parent.score)
            {
                 queue->entries[parent_position] = left_child;
                 queue->entries[left_child_position] = parent;
            }
            break;
        }
        auto right_child = queue->entries[right_child_position];
        if (left_child.score < parent.score)
        {
            if (right_child.score < left_child.score)
            {
                queue->entries[parent_position] = right_child;
                queue->entries[right_child_position] = parent;
                parent_position = right_child_position;
                continue;
            }
            else
            {
                queue->entries[parent_position] = left_child;
                queue->entries[left_child_position] = parent;
                parent_position = left_child_position;
                continue;
            }
        }
        else if (right_child.score < parent.score)
        {
            queue->entries[parent_position] = right_child;
            queue->entries[right_child_position] = parent;
            parent_position = right_child_position;
            continue;
        }
        break;
    }
}

static void
_queue_fix(tile_priority_queue *queue, uint32_t entry_position)
{
    if (entry_position != 0)
    {
        _queue_fix_up(queue, entry_position);
    }
    _queue_fix_down(queue, entry_position);
}

void
queue_update(tile_priority_queue *queue, tile_priority_queue_entry entry)
{
    for (uint32_t i = 0; i < queue->num_entries; ++i)
    {
        auto *current = queue->entries + i;
        if (v2iequal(current->tile_position, entry.tile_position))
        {
            current->score = entry.score;
            _queue_fix(queue, i);
            return;
        }
    }
}
void
queue_add(tile_priority_queue *queue, tile_priority_queue_entry entry)
{
    uint32_t entry_position = queue->num_entries++;
    queue->entries[entry_position] = entry;
    _queue_fix_up(queue, entry_position);
}

tile_priority_queue_entry
queue_pop(tile_priority_queue *queue)
{
    auto entry = queue->entries[0];
    queue->entries[0] = queue->entries[queue->num_entries - 1];
    --queue->num_entries;

    _queue_fix_down(queue, 0);
    return entry;
}

void
queue_init(tile_priority_queue *queue, uint32_t max_entries, tile_priority_queue_entry *entries)
{
    *queue = {
        max_entries,
        0,
        entries,
    };
}

