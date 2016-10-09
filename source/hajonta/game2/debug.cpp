#include "hajonta/algo/fnv1a.cpp"

DebugProfileEventLocation *
find_register_event_location(DebugProfileEventLocationHash *hash, const char *guid)
{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4127) // "conditional expression is constant"
#endif
    hassert(harray_count(hash->locations) == (1<<10));
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    uint32_t hash_loc = fnv1a_10((uint8_t *)guid, (uint32_t)strlen(guid));

    DebugProfileEventLocation *first_item = hash->locations + (hash_loc % harray_count(hash->locations));
    DebugProfileEventLocation *item = first_item;
    if (first_item->name_starts_at)
    {
        do {
            if (strcmp(guid, item->guid) == 0)
            {
                return item;
            }
            item = hash->locations + (++hash_loc % harray_count(hash->locations));
            hassert(item != first_item); // hash full
        } while (item->name_starts_at);
    }
    hassert(strlen(guid) < harray_count(item->guid));
    strcpy(item->guid, guid);
    char *between_file_and_number = strchr(item->guid, '|');
    hassert(between_file_and_number);
    char *between_number_and_name = strchr(between_file_and_number + 1, '|');
    hassert(between_number_and_name);
    item->file_name_starts_at = 0;
    item->file_name_length = (uint32_t)(between_file_and_number - (item->guid + item->file_name_starts_at));
    item->name_starts_at = (uint32_t)(between_number_and_name - item->guid + 1);
    item->line_number = (uint32_t)strtol(between_file_and_number + 1, 0, 10);
    hassert(item->line_number);
    return item;
}

char *
profiling_graph(uint32_t depth, DebugFrame *frame, int32_t zoom_event_offset)
{
    ImGui::PushID((void *)(intptr_t)zoom_event_offset);
    char *guid_clicked = 0;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        ImGui::PopID();
        return guid_clicked;
    }
    ImGuiContext *g = ImGui::GetCurrentContext();
    const ImGuiStyle& style = g->Style;

    uint32_t start_at = 0;
    uint32_t start_depth = 0;
    uint64_t start;
    float divisor;

    if (zoom_event_offset < 0)
    {
        ImGui::Text("Frame %d, %0.4f ms", frame->frame_id, frame->seconds_elapsed * 1000);
        divisor = (float)(frame->end_cycles - frame->start_cycles);
        start = frame->start_cycles;
    }
    else
    {
        float seconds_per_cycle = frame->seconds_elapsed / (frame->end_cycles - frame->start_cycles);
        DebugProfileEvent *zoom_event = frame->events + zoom_event_offset;
        divisor = (float)(zoom_event->end_cycles - zoom_event->start_cycles);
        ImGui::Text("Event %s, ~%0.4f ms", zoom_event->location->guid, divisor * seconds_per_cycle * 1000);
        start = zoom_event->start_cycles;
        start_depth = zoom_event->depth;
        start_at = (uint32_t)zoom_event_offset;
    }

    ImGui::PushItemWidth(-1.0f);
    const ImVec2 graph_size = {
        ImGui::CalcItemWidth(),
        style.IndentSpacing * depth,
    };

    const ImGuiID id = window->GetID("#graph");

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
    ImGui::ItemSize(frame_bb, style.FramePadding.y);
    ImGui::PopItemWidth();
    if (!ImGui::ItemAdd(frame_bb, &id))
    {
        ImGui::PopID();
        return guid_clicked;
    }
    ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    ImVec4 colors[] =
    {
        { 0.3f, 0.0f, 0.0f, 0.8f, },
        { 0.0f, 0.3f, 0.0f, 0.8f, },
        { 0.0f, 0.0f, 0.3f, 0.8f, },
        { 0.3f, 0.3f, 0.0f, 0.8f, },
        { 0.0f, 0.3f, 0.3f, 0.8f, },
        { 0.3f, 0.0f, 0.3f, 0.8f, },
        { 0.2f, 0.2f, 0.2f, 0.8f, },
        { 0.3f, 0.1f, 0.0f, 0.8f, },
        { 0.0f, 0.3f, 0.1f, 0.8f, },
        { 0.1f, 0.0f, 0.3f, 0.8f, },
        { 0.3f, 0.3f, 0.1f, 0.8f, },
        { 1.0f, 0.3f, 0.3f, 0.8f, },
        { 0.3f, 1.0f, 0.3f, 0.8f, },
    };

    for (uint32_t i = start_at; i < frame->event_count; ++i)
    {
        ImGui::PushID((void *)(intptr_t)i);
        const ImGuiID event_id = window->GetID("#box");
        ImGui::PopID();
        DebugProfileEvent *profile_event = frame->events + i;
        uint32_t event_depth = profile_event->depth - start_depth;
        if (event_depth >= depth)
        {
            continue;
        }
        float x_start = (profile_event->start_cycles - start) / divisor * graph_size.x;
        float x_end = (profile_event->end_cycles - start) / divisor * graph_size.x;
        const ImRect event_bb(
            frame_bb.Min + ImVec2(x_start, style.IndentSpacing * event_depth),
            frame_bb.Min + ImVec2(x_end, style.IndentSpacing * (event_depth + 1))
        );

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(event_bb, event_id, &hovered, &held);
        if (pressed)
        {
            guid_clicked = profile_event->location->guid;
        }

        ImGui::RenderFrame(
            event_bb.Min,
            event_bb.Max,
            ImGui::GetColorU32(colors[i % harray_count(colors)]),
            true,
            style.FrameRounding
        );
        //ImGui::Text("Profile event %s: %d self cycles, %d children cycles",
        //        profile_event->location->guid, profile_event->cycles_self, profile_event->cycles_children);
        char *label = profile_event->location->guid + profile_event->location->name_starts_at;
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        ImGui::RenderTextClipped(event_bb.Min, event_bb.Max, label, NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
    }
    ImGui::PopID();
    return guid_clicked;
}

void
show_profiling(DebugSystem *debug_system)
{
    // We're on the first frame, no full frames yet
    if (debug_system->oldest_frame == debug_system->current_frame)
    {
        return;
    }

    if (!debug_system->show)
    {
        return;
    }

    ImGui::Begin("Profiling", &debug_system->show);
    uint32_t previous_frame_offset = 0;
    if (debug_system->current_frame == 0)
    {
        previous_frame_offset = harray_count(debug_system->frames) - 1;
    }
    else
    {
        previous_frame_offset = debug_system->current_frame - 1;
    }
    previous_frame_offset -= previous_frame_offset % 10;
    DebugFrame *previous_frame = debug_system->frames + previous_frame_offset;
    char *new_zoom_guid = profiling_graph(2, previous_frame, -1);
    if (new_zoom_guid)
    {
        debug_system->zoom_guid = new_zoom_guid;
    }
    if (debug_system->zoom_guid)
    {
        int32_t zoom_offset = -1;
        for (uint32_t i = 0; i < previous_frame->event_count; ++i)
        {
            DebugProfileEvent *profile_event = previous_frame->events + i;
            if (strcmp(debug_system->zoom_guid, profile_event->location->guid) == 0)
            {
                zoom_offset = (int32_t)i;
                break;
            }
        }
        if (zoom_offset >= 0)
        {
            new_zoom_guid = profiling_graph(3, previous_frame, zoom_offset);
            if (new_zoom_guid)
            {
                debug_system->zoom_guid = new_zoom_guid;
            }
        }
    }

    for (uint32_t i = 0; i < previous_frame->opengl_timer_count; ++i)
    {
        OpenGLTimerResult *timer = previous_frame->opengl_timer + i;
        ImGui::Text("%s took %f ms", timer->location->guid, (float)timer->result / 1024.0f / 1024.0f);
    }
    ImGui::End();
}

extern "C" GAME_DEBUG_FRAME_END(game_debug_frame_end)
{
    TIMED_FUNCTION();
    game_state *state = (game_state *)memory->memory;
    uint32_t index_count = (uint32_t)GlobalDebugTable->event_index_count;
    uint32_t replacement_index_count = ~(index_count >> 31) << 31;
    index_count = (uint32_t)std::atomic_exchange(
        &GlobalDebugTable->event_index_count,
        (int32_t)replacement_index_count
    );
    uint32_t index = index_count >> 31;
    index_count &= 0x7FFFFFFF;

    auto *debug_system = &state->debug.debug_system;
    DebugFrame *previous_frame = 0;
    auto *current_frame = debug_system->frames + debug_system->current_frame;
    if (debug_system->oldest_frame != debug_system->current_frame)
    {
        previous_frame = debug_system->frames + debug_system->previous_frame;
    }

    for (uint32_t i = 0; i < index_count; ++i)
    {
        DebugEvent *event = GlobalDebugTable->events[index] + i;
        if (!current_frame->start_cycles)
        {
            current_frame->start_cycles = event->tsc_cycles;
        }
        switch (event->type)
        {
            case DebugType::FrameMarker:
            {
                frame_end(debug_system, event->tsc_cycles, event->framemarker.seconds);
                current_frame = debug_system->frames + debug_system->current_frame;
                previous_frame = debug_system->frames + debug_system->previous_frame;
            } break;
            case DebugType::BeginBlock:
            {
                uint32_t event_counter = current_frame->event_count++;
                DebugProfileEvent *profile_event = current_frame->events + event_counter;
                profile_event->location = find_register_event_location(&debug_system->location_hash, event->guid);
                profile_event->start_cycles = event->tsc_cycles;
                profile_event->cycles_self = 0;
                profile_event->cycles_children = 0;
                profile_event->depth = current_frame->parent_count;
                current_frame->parents[current_frame->parent_count++] = event_counter;
            } break;
            case DebugType::EndBlock:
            {
                uint32_t event_counter = current_frame->parents[--current_frame->parent_count];
                DebugProfileEvent *profile_event = current_frame->events + event_counter;
                profile_event->end_cycles = event->tsc_cycles;
                uint64_t all_cycles = profile_event->end_cycles - profile_event->start_cycles;
                profile_event->cycles_self = all_cycles - profile_event->cycles_children;

                if (current_frame->parent_count)
                {
                    uint32_t parent_event_counter = current_frame->parents[current_frame->parent_count - 1];
                    DebugProfileEvent *parent_profile_event = current_frame->events + parent_event_counter;
                    parent_profile_event->cycles_children += all_cycles;
                }
            } break;
            case DebugType::OpenGLTimerResult:
            {
                OpenGLTimerResult *timer = current_frame->opengl_timer + current_frame->opengl_timer_count++;
                timer->location = find_register_event_location(&debug_system->location_hash, event->guid);
                timer->result = event->opengl_timer_result.result;
            } break;
        }
    }

    show_profiling(debug_system);
}
