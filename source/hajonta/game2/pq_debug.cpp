#pragma once

#include "hajonta/game2/pq.h"

void
show_pq_debug(priority_queue_debug *pq)
{
    if (pq->show)
    {
        ImGui::Begin("Priority Queue", &pq->show);
        if (pq->show)
        {
            ImGui::DragFloat("Next value", &pq->value, 0.5f, 1.0f, 100.0f);
            if (ImGui::Button("Add"))
            {
                queue_add(&pq->queue, { pq->value, {0,0} });
            }

            if (ImGui::Button("Test Values"))
            {
                pq->queue.num_entries = 0;
                queue_add(&pq->queue, { 0.5f, {0,0} });
                queue_add(&pq->queue, { 1.5f, {0,0} });
                queue_add(&pq->queue, { 2.5f, {0,0} });
                queue_add(&pq->queue, { 0.25f, {0,0} });
                queue_add(&pq->queue, { 0.75f, {0,0} });
                queue_add(&pq->queue, { 0.1f, {0,0} });
            }

            if (ImGui::Button("Extract"))
            {
                pq->num_sorted = pq->queue.num_entries;
                for (uint32_t i = 0; i < pq->num_sorted; ++i)
                {
                    pq->sorted[i] = queue_pop(&pq->queue).score;
                }
            }

            if (pq->queue.num_entries)
            {
                pq->selected = pq->selected < pq->queue.num_entries ? pq->selected : 0;
                ImGui::Text("Queue has %d of %d entries", pq->queue.num_entries, pq->queue.max_entries);
                for (uint32_t i = 0; i < pq->queue.num_entries; ++i)
                {
                    ImGui::PushID((int32_t)i);
                    char label[20];
                    sprintf(label, "%d: %f", i, pq->entries[i].score);
                    if (ImGui::Selectable(label, pq->selected == i))
                    {
                         pq->selected = i;
                    }
                    ImGui::PopID();
                }
            }

            if (pq->num_sorted)
            {
                ImGui::Text("Sorted list has %d entries", pq->num_sorted);
                for (uint32_t i = 0; i < pq->num_sorted; ++i)
                {
                    ImGui::Text("%d: %f", i, pq->sorted[i]);
                }
            }
        }
        ImGui::End();
    }
}
