#pragma once

static const demo_data menu_items[] = {
    {"Menu", (void *)demo_menu},
    {"Rainbow", (void *)demo_rainbow},
    {"Normals", (void *)demo_normals},
    {"Collision", (void *)demo_collision},
    {"Bounce", (void *)demo_bounce},
};

#include "hajonta/demos/rainbow.cpp"
#include "hajonta/demos/bounce.cpp"
#include "hajonta/demos/normals.cpp"
#include "hajonta/demos/collision.cpp"
#include "hajonta/demos/menu.cpp"
