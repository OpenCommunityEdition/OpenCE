/* Preserve the modern panels' rounded ends as their middle grows. The engine
 * supplies its original Xbox plasma parameters, including screen-anchored waves. */
#include "cseries.h"
#include "frontend_menu.h"
#include "rasterizer/rasterizer.h"
#include "tag_files/tag_files.h"
#include <string.h>

static int frontend_panel_active;

void frontend_panel_begin(long tag)
{
    char const *name;
    frontend_panel_active = 0;
    if (!frontend_menu_available()) return;
    name = tag_get_name(tag);
    if (!name || strncmp(name, "pc\\modern\\", 10)) return;
    name = strrchr(name, '\\') + 1;
    frontend_panel_active = !strncmp(name, "row_", 4) || !strcmp(name, "help_panel");
    if (!strncmp(tag_get_name(tag), "pc\\modern\\network\\", 18))
        frontend_panel_active |= !strncmp(name, "op_", 3) ||
            (!strncmp(name, "list_item_", 10) && name[10] >= '0' && name[10] <= '9');
}

void frontend_panel_end(void)
{
    frontend_panel_active = 0;
}

int frontend_panel_draw(struct rasterizer_dynamic_screen_geometry_parameters *parameters,
    struct dynamic_screen_vertex *vertices, long source_width)
{
    struct dynamic_screen_vertex part[4];
    real x[4], u[4], cap = 18.0f;
    int slice, i;
    if (!frontend_panel_active || source_width <= 2 * cap ||
        vertices[1].position.x - vertices[0].position.x <= 2 * cap) return 0;
    x[0] = vertices[0].position.x;
    x[1] = x[0] + cap;
    x[3] = vertices[1].position.x;
    x[2] = x[3] - cap;
    u[0] = vertices[0].texture_coordinates.x;
    u[3] = vertices[1].texture_coordinates.x;
    u[1] = u[0] + (u[3] - u[0]) * cap / source_width;
    u[2] = u[3] - (u[3] - u[0]) * cap / source_width;
    for (slice = 0; slice < 3; slice++)
    {
        memcpy(part, vertices, sizeof(part));
        for (i = 0; i < 4; i++)
        {
            int right = ((i + 1) & 2) != 0;
            part[i].position.x = x[slice + right];
            part[i].texture_coordinates.x = u[slice + right];
        }
        rasterizer_psuedo_dynamic_screen_quad_draw(parameters, part);
    }
    return 1;
}
