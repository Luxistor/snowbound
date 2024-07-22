#include "sb_app.h"
#include "sb_file.h"
#include "sb_timer.h"
#include "sb_string.h"
#include "sb_math.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef enum
{
    DOOR_COLOR_PURPLE,
    DOOR_COLOR_BLUE,
    DOOR_COLOR_GREEN,
    DOOR_COLOR_COUNT,
} door_color_t;

sb_color3 door_color_as_color3(door_color_t color)
{
    switch(color)
    {
        case DOOR_COLOR_PURPLE: return SB_PURPLE;
        case DOOR_COLOR_BLUE: return SB_BLUE;
        case DOOR_COLOR_GREEN: return SB_GREEN;
    }
}

typedef enum
{
    BUTTON_COLOR_ROSE,
    BUTTON_COLOR_YELLOW,
    BUTTON_COLOR_ORANGE,
    BUTTON_COLOR_COUNT,
} button_color_t;

sb_color3 button_color_as_color3(button_color_t color)
{
    switch(color)
    {
        case BUTTON_COLOR_ROSE: return SB_ROSE;
        case BUTTON_COLOR_YELLOW: return SB_YELLOW;
        case BUTTON_COLOR_ORANGE: return SB_ORANGE;
    }
}

typedef enum
{
    TELEPORT_COLOR_GREEN,
    TELEPORT_COLOR_BLUE,
    TELEPORT_COLOR_RED,
    TELEPORT_COLOR_COUNT,
} teleport_color_t;

sb_color3 teleport_color_as_color3(teleport_color_t color)
{
    switch(color)
    {
        case TELEPORT_COLOR_GREEN: return SB_GREEN;
        case TELEPORT_COLOR_BLUE: return SB_LIGHT_BLUE;
        case TELEPORT_COLOR_RED: return SB_RED;
    }
}

typedef enum
{
    TILE_TYPE_NONE,
    TILE_TYPE_ICE,
    TILE_TYPE_WATER,
    TILE_TYPE_WALL,
    TILE_TYPE_DOOR,
    TILE_TYPE_PILLAR,
    TILE_TYPE_TELEPORT,
    TILE_TYPE_BEGIN,
    TILE_TYPE_BUTTON,
    TILE_TYPE_END,
} tile_type_t;

bool tile_is_elevated(tile_type_t type)
{
    return type == TILE_TYPE_WALL || type == TILE_TYPE_DOOR;
}

typedef enum
{
    PICKUP_TYPE_NONE,
    PICKUP_TYPE_KEY,
    PICKUP_TYPE_SPEED,
    PICKUP_TYPE_MONEY,
    PICKUP_TYPE_BOULDER,
    PICKUP_TYPE_PLAYER,
} pickup_type_t;

typedef struct tile_t tile_t; 
struct tile_t
{
    tile_type_t tile_type;
    pickup_type_t pickup_type; // whats on top of the tile

    tile_t *pillar;

    uint8_t u8;

    sb_ivec2 direction;
    sb_ivec2 teleport_pos;

    bool b32;
    float f32;

    sb_mat4 transform;
    sb_timer move_timer;
};

#define CUBE_SIDE_LENGTH 2.0
#define VEC3_ELEVATION (sb_vec3) {0, CUBE_SIDE_LENGTH, 0}

sb_vec3 get_world_position(sb_ivec2 in)
{
    return sb_vec3_mul_f32((sb_vec3){.x = in.x, 0, .z = in.y}, CUBE_SIDE_LENGTH);
}

bool tile_is_moving(const tile_t *tile)
{
    return !sb_ivec2_eq(tile->direction, (sb_ivec2) {0});
}

bool tile_can_move(const tile_t *tile)
{
    return (tile->pickup_type == PICKUP_TYPE_PLAYER || tile->pickup_type == PICKUP_TYPE_BOULDER);
}

void stop_tile_moving(tile_t *tile)
{
    tile->direction = (sb_ivec2) {0};
    tile->pickup_type = PICKUP_TYPE_NONE;
}

void init_boulder(tile_t *tile, sb_ivec2 direction)
{
    tile->direction = direction;
    tile->pickup_type = PICKUP_TYPE_BOULDER;
    tile->move_timer = sb_create_timer(0.3f);
}

void init_player(tile_t *player_tile)
{
    player_tile->move_timer = sb_create_timer(0.35f);
    player_tile->pickup_type = PICKUP_TYPE_PLAYER;
}

void set_player_transform(sb_mat4 out, sb_ivec2 player_pos)
{
    sb_vec3 start_pos = sb_vec3_add(VEC3_ELEVATION, get_world_position(player_pos));
    sb_mat4_from_position(out, start_pos);
}

typedef enum
{
    PLAYER_STATE_AVAILABLE,
    PLAYER_STATE_MOVING,
    PLAYER_STATE_DYING,
} player_state_t;

typedef struct
{
    uint8_t height;
    uint8_t width;
    sb_ivec2 start_pos;
    tile_t tiles[];
} level_t;

bool increment_pos(const level_t *level, sb_ivec2 *pos)
{
    if(pos->x != (level->width - 1))
    {
        pos->x++;
        return true;
    }

    if(pos->y == (level->height -1))return false;
    else
    {
        pos->y++;
        pos->x = 0;
        return true;
    }

}

bool in_level_bounds(level_t *level, int x, int y)
{
    return x >= 0 && x < level->width && y >= 0 && y < level->height;
}

uint32_t get_tile_index(level_t *level, sb_ivec2 pos)
{
    return pos.y*level->width + pos.x;
}

tile_t *get_tile(level_t *level, sb_ivec2 pos)
{
    if(!in_level_bounds(level, pos.x, pos.y)) return NULL;
    return &level->tiles[get_tile_index(level, pos)];
}

#define LEVEL_COUNT 4U

bool tile_is_blocking(tile_t *tile)
{
    return     (tile->tile_type == TILE_TYPE_DOOR
            || (tile->tile_type == TILE_TYPE_PILLAR && tile->b32)
            || tile->tile_type == TILE_TYPE_WALL
            || tile->tile_type == TILE_TYPE_WATER);
}

void move_tiles(level_t *level, float dt)
{
    for(sb_ivec2 pos = {0}; increment_pos(level, &pos);)
    {
        tile_t *tile = get_tile(level, pos);

        if(tile_is_moving(tile))
        {
            sb_ivec2 next_pos = sb_ivec2_add(pos, tile->direction);

            sb_vec3 world_pos = sb_vec3_add(sb_vec3_lerp(
                get_world_position(pos),
                get_world_position(next_pos),
                1 - sb_timer_percent_left(&tile->move_timer)
            ), VEC3_ELEVATION);

            sb_mat4_set_position(tile->transform, world_pos);

            if(sb_timer_tick(&tile->move_timer, dt))
            {
                tile_t *next_tile = get_tile(level, next_pos);

                if(next_tile->tile_type == TILE_TYPE_TELEPORT)
                {
                    tile_t *teleport_tile = get_tile(level, next_tile->teleport_pos);
                    sb_vec3 new_pos = sb_vec3_add(VEC3_ELEVATION, get_world_position(next_tile->teleport_pos));
                    
                    sb_mat4_from_position(teleport_tile->transform, new_pos);
                    next_pos = next_tile->teleport_pos;
                    next_tile = teleport_tile;
                }
                else
                    sb_mat4_copy(tile->transform, next_tile->transform);

                if(tile->tile_type == TILE_TYPE_BUTTON)
                    tile->pillar->b32 = true;
                if(next_tile->tile_type == TILE_TYPE_BUTTON)
                    next_tile->pillar->b32 = false;

                if(tile->pickup_type == PICKUP_TYPE_BOULDER)
                {
                    tile_t *boulder_next = get_tile(level, sb_ivec2_add(next_pos, tile->direction));

                    if(boulder_next && !tile_is_blocking(boulder_next))
                        next_tile->direction = tile->direction;
                }

                next_tile->pickup_type = tile->pickup_type;
                next_tile->move_timer = sb_create_timer(tile->move_timer.duration);

                tile->pickup_type = PICKUP_TYPE_NONE;
                tile->direction = (sb_ivec2) {0};
            }

        }

    }
}

void update_pillars(level_t *level, float dt)
{
    for(sb_ivec2 pos = {0}; increment_pos(level, &pos);)
    {
        tile_t *tile = get_tile(level ,pos);
        if(tile->tile_type == TILE_TYPE_PILLAR)
        {
            bool is_active = tile->b32;
            float translation_y = tile->f32;

            float increment = 0;
            if(is_active)
            {
                if(translation_y < 0)
                    increment = dt;
            }
            else
            {
                if(translation_y > -CUBE_SIDE_LENGTH)
                    increment = -dt;
            }

            tile->f32 += increment*2;
        }

    }
}

void update_teleport_colors(level_t *level, float dt)
{
    for(sb_ivec2 pos = {0}; increment_pos(level, &pos);)
    {
        tile_t *tile = get_tile(level, pos);

        if(tile->tile_type == TILE_TYPE_TELEPORT)
        {   
            bool is_up = tile->b32;
            float t = tile->f32;

            if(t > 0.98f)
                is_up = false;
            else if (t < 0.01f)
                is_up = true;

            tile->f32 += (is_up ? 1 : -1)*dt/2;
            tile->b32 = is_up;
        }
    }
}

void rotate_keys(level_t *level, float dt)
{
    for(sb_ivec2 pos = {0}; increment_pos(level, &pos);)
    {
        tile_t *tile = get_tile(level ,pos);
        if(tile->pickup_type == PICKUP_TYPE_KEY)
            tile->f32 += dt;

    }

}

const sb_keycode input_keys[] = {SB_KEY_CODE_W, SB_KEY_CODE_A, SB_KEY_CODE_S, SB_KEY_CODE_D};

typedef struct
{
    sb_arena *arena;

    sb_arena_temp reset_point;
    sb_str8 level_text;

    uint8_t level_number;
    level_t *level;

    float scroll_pos;
    float screen_shake;

    sb_keycode input_stack[COUNTOF(input_keys)];
    uint8_t input_stack_count;

    uint32_t score;
    bool keys_received[DOOR_COLOR_COUNT];

    player_state_t player_state;
    sb_ivec2 player_pos;
    sb_ivec2 next_player_pos;
    sb_timer player_death_timer;
} game_state_t;

tile_t *get_player_tile(game_state_t *game_state)
{
    return get_tile(game_state->level, game_state->player_pos);
}

typedef struct
{
    sb_mesh_id cube_mesh;
    sb_mesh_id water_mesh;
    sb_mesh_id player_mesh;
    sb_mesh_id key_mesh;
    sb_mesh_id boulder_mesh;
    sb_mesh_id button_mesh;
    sb_mesh_id pillar_mesh;
    sb_mesh_id teleport_pad_mesh;
    sb_mesh_id plane_mesh;

    sb_texture_id pillar_texture;
    sb_texture_id teleport_texture;
    sb_texture_id rock_texture;
    sb_texture_id crate_texture;
    sb_texture_id metal_texture;
    sb_texture_id ice_texture;
    sb_texture_id player_texture;
    sb_texture_id door_texture;
} assets_t;

bool tile_neighbors_water(level_t *level, sb_ivec2 pos)
{

    for(int x = -1; x <= 1; x++)
    {
        for(int y = -1; y <= 1; y++)
        {
            sb_ivec2 neighbor = sb_ivec2_add(pos, (sb_ivec2) {x,y});

            tile_t *tile = get_tile(level, neighbor);
            if(tile && tile->tile_type == TILE_TYPE_WATER) return true;
        } 
    }

    return false;
}

assets_t load_assets(sb_app *app)
{
    assets_t assets = {0};

    assets.cube_mesh = sb_create_mesh(app, "assets/meshes/cube.sbm");
    assets.water_mesh = sb_create_mesh(app, "assets/meshes/water.sbm");
    assets.player_mesh = sb_create_mesh(app, "assets/meshes/box.sbm");
    assets.key_mesh = sb_create_mesh(app, "assets/meshes/key.sbm");
    assets.boulder_mesh = sb_create_mesh(app, "assets/meshes/crate.sbm");
    assets.button_mesh = sb_create_mesh(app, "assets/meshes/button.sbm");
    assets.pillar_mesh = sb_create_mesh(app, "assets/meshes/pillar.sbm");
    assets.teleport_pad_mesh = sb_create_mesh(app, "assets/meshes/pad.sbm");
    assets.plane_mesh = sb_create_mesh(app, "assets/meshes/ice.sbm");

    assets.teleport_texture = sb_texture_from_file(app, "assets/textures/teleport.png");
    assets.pillar_texture = sb_texture_from_file(app, "assets/textures/pillar.png");
    assets.metal_texture = sb_texture_from_file(app, "assets/textures/metal.png");
    assets.rock_texture = sb_texture_from_file(app, "assets/textures/marble.png");
    assets.crate_texture = sb_texture_from_file(app, "assets/textures/crate.jpeg");
    assets.ice_texture = sb_texture_from_file(app, "assets/textures/ice.jpg");
    assets.player_texture = sb_texture_from_file(app, "assets/textures/rock.jpeg");
    assets.door_texture = sb_texture_from_file(app, "assets/textures/door.jpg");

    return assets;
}

void draw_ice(sb_app *app, assets_t *assets, sb_vec3 position)
{
    sb_draw_info draw_info = {0};
    draw_info.color = sb_color3_as_u32(SB_WHITE);
    draw_info.specularity = 10000;
    draw_info.texture_id = assets->ice_texture;
    draw_info.mesh_id = assets->cube_mesh;
    sb_mat4_from_position(draw_info.transform, position);
    sb_draw(app, &draw_info);
}

void draw_metal(sb_app *app, assets_t *assets, sb_vec3 position)
{
    sb_draw_info metal_info = {0};
    sb_mat4_from_position(metal_info.transform, position);
    metal_info.color = sb_color3_as_u32(SB_WHITE);
    metal_info.mesh_id = assets->cube_mesh;
    metal_info.texture_id = assets->metal_texture;
    sb_draw(app, &metal_info);
}

void draw_level(sb_app *app, const assets_t *assets, level_t *level)
{
    for(sb_ivec2 pos = {0}; increment_pos(level, &pos);)
    {
        const tile_t *tile = get_tile(level, pos);
        sb_vec3 world_position = get_world_position(pos);

        {
            sb_vec3 elevation = tile_is_elevated(tile->tile_type) ? VEC3_ELEVATION : (sb_vec3) {0};
            sb_draw_info draw_info = {0};
            draw_info.color = sb_color3_as_u32(SB_WHITE);
            sb_mat4_from_position(draw_info.transform, (sb_vec3) {pos.x * 2, elevation.y, pos.y * 2});

            draw_info.mesh_id = assets->cube_mesh;

            switch(tile->tile_type)
            {   
                case TILE_TYPE_BUTTON:
                    draw_info.color = sb_color3_as_u32(button_color_as_color3(tile->u8));
                    draw_info.mesh_id = assets->button_mesh;

                    draw_metal(app, assets, world_position);
                    break;
                case TILE_TYPE_TELEPORT:
                    draw_info.mesh_id = assets->teleport_pad_mesh;
                    draw_info.specularity = 2048.0f;
                    draw_info.texture_id = assets->teleport_texture;

                    sb_color3 start = teleport_color_as_color3(tile->u8);
                    sb_color3 goal = SB_WHITE;

                    sb_color3 color = sb_color3_lerp(start, goal, tile->f32);

                    draw_info.color = sb_color3_as_u32(color);

                    draw_metal(app, assets, world_position);
                    break;
                case TILE_TYPE_PILLAR:
                    draw_info.color = sb_color3_as_u32(button_color_as_color3(tile->u8));
                    draw_info.texture_id = assets->pillar_texture;
                    sb_mat4_translate(draw_info.transform, (sb_vec3) { 0, tile->f32, 0 });
                    draw_info.mesh_id = assets->pillar_mesh;

                    draw_metal(app, assets, world_position);
                    break;
                case TILE_TYPE_BEGIN:
                    draw_info.color = sb_color3_as_u32((sb_color3) {0,153,16});
                    break;
                case TILE_TYPE_END:
                    draw_info.color = sb_color3_as_u32((sb_color3){128,27,27});
                    break;
                case TILE_TYPE_WALL:
                    draw_info.color = sb_color3_as_u32(SB_LIGHT_BLUE);
                    draw_info.texture_id = assets->rock_texture;
                    draw_ice(app, assets, world_position);
                    break;
                case TILE_TYPE_WATER:
                    draw_info.color = sb_color3_as_u32((sb_color3) {1,16,104});
                    draw_info.shader_id = 1;
                    draw_info.specularity = 1000.0f;
                    draw_info.mesh_id = assets->water_mesh;
                    break;
                case TILE_TYPE_ICE:
                    draw_info.specularity = 2048.0f;
                    draw_info.shader_id = 2;
                    draw_info.mesh_id = tile_neighbors_water(level, pos) ? assets->cube_mesh : assets->plane_mesh;
                    draw_info.texture_id = assets->ice_texture;
                    break;
                case TILE_TYPE_DOOR:
                    draw_info.specularity = 50.0f;
                    draw_info.color = sb_color3_as_u32(door_color_as_color3(tile->u8));
                    draw_info.texture_id = assets->door_texture;
                    draw_ice(app, assets, world_position);
                    break;
            }

            if(tile->tile_type != TILE_TYPE_NONE)
                sb_draw(app, &draw_info);
        }

        {
            sb_draw_info draw_info = {0};

            switch(tile->pickup_type)
            {
                case PICKUP_TYPE_BOULDER:
                    sb_mat4_copy(tile->transform, draw_info.transform);
                    draw_info.color = sb_color3_as_u32(SB_BROWN);
                    draw_info.mesh_id = assets->boulder_mesh;
                    draw_info.texture_id = assets->crate_texture;
                    sb_draw(app, &draw_info);
                    break;
                case PICKUP_TYPE_PLAYER:
                    sb_mat4_copy(tile->transform, draw_info.transform);
                    draw_info.color = sb_color3_as_u32(SB_WHITE);
                    draw_info.mesh_id  = assets->cube_mesh;
                    draw_info.texture_id = assets->player_texture;
                    sb_draw(app, &draw_info);
                    break;
                case PICKUP_TYPE_KEY:
                    sb_mat4_from_angles(draw_info.transform, (sb_vec3) {0, tile->f32, 0});

                    sb_vec3 translate = sb_vec3_add(world_position, VEC3_ELEVATION);
                    sb_mat4_translate(draw_info.transform, translate);

                    draw_info.color = sb_color3_as_u32(door_color_as_color3(tile->u8));
                    draw_info.mesh_id  = assets->key_mesh;
                    draw_info.specularity = 2048.0f;
                    sb_draw(app, &draw_info);
                 break;
            }
        }
    }

}

level_t *get_level(sb_arena *arena, sb_str8 level_text)
{
    uint8_t width = 0; uint8_t height = 1;

    {
        uint8_t row_width = 0;
        for(int i = 0; i < level_text.size; i++)
        {
            char c = level_text.str[i];
            if(c == '\n')
            {
                if(row_width > width) width = row_width;

                row_width = 0;
                height++;
                continue;
            }

            row_width++;
        }
    }

    level_t *level = sb_arena_push_aligned(arena, sizeof(level_t) + width*height*sizeof(tile_t), 64);
    level->width = width;
    level->height = height;

    int x = 0; int y = 0;

    tile_t *buttons[BUTTON_COLOR_COUNT] = {0};
    tile_t *pillars[BUTTON_COLOR_COUNT] = {0};

    bool found_teleport[TELEPORT_COLOR_COUNT] = {0};
    sb_ivec2 teleports[TELEPORT_COLOR_COUNT] = {0};

    for(int i = 0; i < level_text.size; i++)
    {
        char c = level_text.str[i];

        tile_t *tile = &level->tiles[y*width + x];
        sb_ivec2 pos ={x,y};

        tile_type_t tile_type = TILE_TYPE_NONE;
        pickup_type_t pickup_type = PICKUP_TYPE_NONE;

        #define SET_KEY(color) tile->u8 = color; pickup_type = PICKUP_TYPE_KEY; tile_type = TILE_TYPE_ICE;
        #define SET_DOOR(color) tile->u8 = color ; tile_type = TILE_TYPE_DOOR;
        #define SET_BUTTON(color) tile->u8 = color; tile_type = TILE_TYPE_BUTTON; buttons[color] = tile;
        #define SET_PILLAR(color) tile->u8 = color; tile_type = TILE_TYPE_PILLAR; pillars[color] = tile; tile->b32 = true;
        #define SET_TELEPORT(color)\
                tile_type = TILE_TYPE_TELEPORT;\
                tile->u8 = color;\
                if(!found_teleport[color])\
                {\
                    found_teleport[color] = true;\
                    teleports[color] = pos;\
                }\
                else\
                {\
                    sb_ivec2 other = teleports[color];\
                    tile->teleport_pos = other;\
                    tile_t *link = get_tile(level, other);\
                    link->teleport_pos = pos;\
                }

        switch(c)
        {
            case '#': tile_type = TILE_TYPE_WALL;                          break;
            case 'S': tile_type = TILE_TYPE_BEGIN; level->start_pos = pos; break;
            case 'E': tile_type = TILE_TYPE_END;                           break;
            case 'C':
                tile_type = TILE_TYPE_ICE;
                pickup_type = PICKUP_TYPE_BOULDER;
                sb_mat4_from_position(tile->transform, sb_vec3_add(VEC3_ELEVATION, get_world_position(pos)));
                break;
            case 'P': SET_DOOR(DOOR_COLOR_PURPLE)     break;
            case 'B': SET_DOOR(DOOR_COLOR_BLUE)       break;
            case 'G': SET_DOOR(DOOR_COLOR_GREEN)      break;
            case 'p': SET_KEY(DOOR_COLOR_PURPLE)      break;
            case 'b': SET_KEY(DOOR_COLOR_BLUE)        break;
            case 'g': SET_KEY(DOOR_COLOR_GREEN)       break;
            case 'o': SET_BUTTON(BUTTON_COLOR_ORANGE) break;
            case 'r': SET_BUTTON(BUTTON_COLOR_ROSE)   break;
            case 'y': SET_BUTTON(BUTTON_COLOR_YELLOW) break;
            case 'O': SET_PILLAR(BUTTON_COLOR_ORANGE) break;
            case 'R': SET_PILLAR(BUTTON_COLOR_ROSE)   break;
            case 'Y': SET_PILLAR(BUTTON_COLOR_YELLOW) break;
            case '0': SET_TELEPORT(0)                 break;
            case '1': SET_TELEPORT(1)                 break;
            case '2': SET_TELEPORT(2)                 break;
            case '-': tile_type = TILE_TYPE_ICE;      break;

        }

        #undef SET_BUTTON
        #undef SET_DOOR
        #undef SET_PILLAR
        #undef SET_TELEPORT
        #undef SET_KEY

        for(button_color_t color = 0; color < BUTTON_COLOR_COUNT; color++)
        {   
            tile_t *button = buttons[color];
            if(!button) continue;
            buttons[color]->pillar = pillars[color];
        }

        tile->tile_type = tile_type;
        tile->pickup_type = pickup_type;

        if (x++ == level->width)
        {
            x = 0;
            y++;
            continue;
        }
    }

    return level;
}

void handle_input(const sb_window_event *event, game_state_t *game_state)
{
    game_state->scroll_pos = sb_clamp(game_state->scroll_pos - (float)event->scroll_delta, -4.0f, 18.0f);

    for(int i = 0; i < COUNTOF(input_keys); i++)
    {
        sb_keycode input_key = input_keys[i];
        if(event->keys_just_pressed[input_key])
        {
            assert(game_state->input_stack_count != COUNTOF(input_keys));

            for(int j = 1; j < game_state->input_stack_count + 1; j++)
            {
                sb_keycode last_keycode = game_state->input_stack[j-1];
                sb_keycode current_keycode = game_state->input_stack[j];

                game_state->input_stack[j] = last_keycode;

                uint8_t next_index = j+1;
                if(next_index < game_state->input_stack_count)
                    game_state->input_stack[next_index] = current_keycode;
            }

            game_state->input_stack[0] = input_key;
            game_state->input_stack_count++;
        }
        else if(event->keys_just_released[input_key])
        {
            int stack_index = -1;
            for(int j = 0; j < game_state->input_stack_count; j++)
            {
                if(game_state->input_stack[j] == input_key) {
                    stack_index = j;
                    break;
                }
            }

            if(stack_index == -1) continue;

            // remove
            for(int j = stack_index; j < game_state->input_stack_count; j++)
            {
                sb_keycode next_keycode = SB_KEY_CODE_NULL;
                int next_index = j+1;

                if(next_index < game_state->input_stack_count)
                    next_keycode = game_state->input_stack[next_index];

                game_state->input_stack[j] = next_keycode;
            }

            game_state->input_stack_count--;
        }
    }
}

sb_ivec2 get_move_direction(sb_keycode key_code)
{
    switch(key_code)
    {
        case SB_KEY_CODE_W: return (sb_ivec2) {1,0};
        case SB_KEY_CODE_A: return (sb_ivec2) {0,-1};
        case SB_KEY_CODE_S: return (sb_ivec2) {-1,0};
        case SB_KEY_CODE_D: return (sb_ivec2) {0,1};
    }
}

sb_str8 get_level_text(sb_arena *arena, uint8_t number)
{
    sb_arena_temp scratch = sb_get_scratch_with_conflicts(&arena, 1);
    sb_str8 number_string = sb_u8_to_str8(scratch.arena, number);
    sb_str8 directory = sb_str8_concat(scratch.arena, sb_str8_lit("assets/levels/"), number_string);
    sb_str8 save_file_name = sb_str8_concat(scratch.arena, directory, sb_str8_lit(".txt"));

    sb_str8 text = sb_read_file_string(arena, save_file_name.str);
    sb_release_scratch(&scratch);
    return text;
}

void reset_game_state(game_state_t *game_state)
{
    sb_arena_temp_end(&game_state->reset_point);
    game_state->level = get_level(game_state->arena, game_state->level_text);

    SB_ZERO_ARRAY(game_state->keys_received);
    game_state->score = 0;

    game_state->player_pos = game_state->level->start_pos;
    game_state->player_death_timer = sb_create_timer(1.5f);
    game_state->player_state = PLAYER_STATE_AVAILABLE;

    tile_t *player_tile = get_player_tile(game_state);
    init_player(player_tile);
    set_player_transform(player_tile->transform, game_state->player_pos);
}

void new_level(game_state_t *game_state)
{
    sb_reset_arena(game_state->arena);
    game_state->level_text = get_level_text(game_state->arena, +game_state->level_number++);
    game_state->reset_point = sb_arena_temp_begin(game_state->arena);
    reset_game_state(game_state);
}

sb_ivec2 directions[] = { {0, 1}, {0, -1}, {1, 0}, {-1, 0} };

void handle_player_state(game_state_t *game_state, float dt)
{
    tile_t *current_tile = get_player_tile(game_state);
    switch(game_state->player_state)
    {
        case PLAYER_STATE_AVAILABLE:
            sb_keycode keycode_pressed = game_state->input_stack[0];
            if(keycode_pressed == SB_KEY_CODE_NULL) return;

            sb_ivec2 move_direction = get_move_direction(keycode_pressed);
            sb_ivec2 next_position = sb_ivec2_add(move_direction, game_state->player_pos);
            tile_t *next_tile = get_tile(game_state->level, next_position);
            if(!next_tile) return;

            switch (next_tile->pickup_type)
            {
                case PICKUP_TYPE_KEY:
                    door_color_t color = next_tile->u8;
                    game_state->keys_received[color] = true;
                    next_tile->pickup_type = PICKUP_TYPE_NONE;
                    break;
                case PICKUP_TYPE_BOULDER:
                    if(tile_is_moving(next_tile)) break;
                    tile_t *boulder_next = get_tile(game_state->level, sb_ivec2_add(move_direction, next_position));
                    if(!boulder_next || tile_is_blocking(boulder_next)) return;
                    init_boulder(next_tile, move_direction);

                    SB_ZERO_ARRAY(game_state->input_stack);
                    game_state->input_stack_count = 0;

                    return;
            }

            switch(next_tile->tile_type)
            {
                case TILE_TYPE_NONE:
                case TILE_TYPE_WALL: return;

                case TILE_TYPE_PILLAR:
                    if(next_tile->b32) return;
                    game_state->player_state = PLAYER_STATE_MOVING;
                    break;

                case TILE_TYPE_DOOR:
                    if(!game_state->keys_received[next_tile->u8]) return;
                    next_tile->tile_type = TILE_TYPE_ICE;
                case TILE_TYPE_BUTTON:
                case TILE_TYPE_TELEPORT:
                case TILE_TYPE_END:
                case TILE_TYPE_ICE:
                case TILE_TYPE_WATER:
                case TILE_TYPE_BEGIN:
                    game_state->player_state = PLAYER_STATE_MOVING;
                    break;
            }

            if (current_tile->tile_type == TILE_TYPE_ICE)
                current_tile->tile_type = TILE_TYPE_WATER;

            game_state->next_player_pos = next_position;

            current_tile->direction = move_direction;
            current_tile->pickup_type = PICKUP_TYPE_PLAYER;

            return;
        case PLAYER_STATE_MOVING:
            if(!sb_timer_just_finished(&current_tile->move_timer)) return;

            game_state->player_pos = game_state->next_player_pos;

            tile_t *new_tile = get_player_tile(game_state);

            switch(new_tile->tile_type)
            {
                case TILE_TYPE_WATER: game_state->player_state = PLAYER_STATE_DYING; return;
                case TILE_TYPE_END:
                    if(game_state->level_number == LEVEL_COUNT) return; // todo: victory

                    sb_reset_arena(game_state->arena);
                    new_level(game_state);
                    return;
                case TILE_TYPE_TELEPORT:
                    game_state->player_pos = new_tile->teleport_pos;
                    game_state->next_player_pos = new_tile->teleport_pos;
            }

            game_state->player_state = PLAYER_STATE_AVAILABLE;
            return;
        case PLAYER_STATE_DYING:
            set_player_transform(current_tile->transform, game_state->player_pos);
            sb_mat4_scale(current_tile->transform, sb_timer_percent_left(&game_state->player_death_timer));
            if(sb_timer_tick(&game_state->player_death_timer, dt))
            {
                sb_arena_temp_end(&game_state->reset_point);
                reset_game_state(game_state);
            }
            return;
    }
}

typedef struct
{
    sb_texture_id depth_buffer_id;
    sb_texture_id normal_texture_id;
    sb_texture_id albedoRGB_specularityA_id;
    sb_texture_id shadow_map_id;
    sb_texture_id ssao_id;
    sb_texture_id ssao_blur_id;

    VkPipeline ssao_pipeline;
    VkPipeline ssao_blur_pipeline;
    VkPipeline gpass_pipeline;
    VkPipeline shadow_pipeline;
    VkPipeline lighting_pipeline;
} resources_t;

sb_image_transition get_fragment_attachment_transition(sb_texture *texture)
{
    sb_image_transition transition = {0};
    transition.texture = texture;
    transition.new_layout = SB_IMAGE_LAYOUT_RENDER_ATTACHMENT;
    transition.src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    transition.dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    return transition;
}

sb_image_transition get_fragment_readonly_transition(sb_texture *texture)
{
    sb_image_transition transition = {0};
    transition.texture = texture;
    transition.old_layout = SB_IMAGE_LAYOUT_RENDER_ATTACHMENT;
    transition.new_layout = SB_IMAGE_LAYOUT_READ_ONLY;
    transition.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    transition.dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    return transition;
}

sb_image_transition get_depth_readonly_transition(sb_texture *texture)
{
    sb_image_transition transition = {0};
    transition.texture = texture;
    transition.texture = texture;
    transition.old_layout = SB_IMAGE_LAYOUT_RENDER_ATTACHMENT;
    transition.new_layout = SB_IMAGE_LAYOUT_READ_ONLY;
    transition.src_stage_mask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    transition.dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    return transition;
}

void bake_command_buffer(sb_app *app, VkCommandBuffer command_buffer, sb_texture *swapchain_texture)
{
    const resources_t *resources = app->user_data;

    sb_texture *depth_buffer                   = sb_get_texture(app, resources->depth_buffer_id);
    sb_texture *normal_texture                 = sb_get_texture(app, resources->normal_texture_id);
    sb_texture *albedoRGB_specularityA_texture = sb_get_texture(app, resources->albedoRGB_specularityA_id);
    sb_texture *shadow_map                     = sb_get_texture(app, resources->shadow_map_id);
    sb_texture *ssao_texture                   = sb_get_texture(app, resources->ssao_id);
    sb_texture *ssao_blur                      = sb_get_texture(app, resources->ssao_blur_id);

    // transition gpass resources and shadow map resources as they can occur concurrently
    {
        sb_image_transition depth_buffer_attachment_transition = {0};
        depth_buffer_attachment_transition.texture = depth_buffer;
        depth_buffer_attachment_transition.new_layout = SB_IMAGE_LAYOUT_RENDER_ATTACHMENT;
        depth_buffer_attachment_transition.src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        depth_buffer_attachment_transition.dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        sb_image_transition shadow_map_transition = {0};
        shadow_map_transition.texture = shadow_map;
        shadow_map_transition.new_layout = SB_IMAGE_LAYOUT_RENDER_ATTACHMENT;
        shadow_map_transition.src_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        shadow_map_transition.dst_stage_mask =  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;;

        sb_image_transition transitions[] = {
            get_fragment_attachment_transition(normal_texture),
            get_fragment_attachment_transition(albedoRGB_specularityA_texture),

            depth_buffer_attachment_transition,
            shadow_map_transition
        };

        sb_set_image_layouts(command_buffer, transitions, COUNTOF(transitions));
    }


    // geometry pass
    {
        VkRenderingAttachmentInfo color_attachments[] = {
            sb_rendering_attachment_info(normal_texture, (sb_clear_value) {0}),
            sb_rendering_attachment_info(albedoRGB_specularityA_texture, (sb_clear_value) {.color = SB_WHITE}),
        };

        VkRenderingAttachmentInfo depth_attachment = sb_rendering_attachment_info(depth_buffer, (sb_clear_value) {.depth = 1.0f} );

        sb_render_pass_info gpass = {0};
        gpass.color_attachments = color_attachments;
        gpass.color_attachment_count = COUNTOF(color_attachments);
        gpass.render_area = swapchain_texture->extent;
        gpass.depth_attachment = &depth_attachment;

        sb_begin_render_pass(command_buffer, &gpass);

        sb_bind_graphics_pipeline(command_buffer, resources->gpass_pipeline);
        sb_draw_scene(command_buffer, &app->indirect_command_buffer);

        sb_end_render_pass(command_buffer);
    }

    // shadow map generation pass
    {
        VkRenderingAttachmentInfo depth_attachment = sb_rendering_attachment_info(shadow_map, (sb_clear_value) {.depth = 1.0f} );

        sb_render_pass_info shadow_pass = {0};
        shadow_pass.depth_attachment = &depth_attachment;
        shadow_pass.render_area = shadow_map->extent;

        sb_begin_render_pass(command_buffer, &shadow_pass);

        sb_bind_graphics_pipeline(command_buffer, resources->shadow_pipeline);
        sb_draw_scene(command_buffer, &app->indirect_command_buffer);

        sb_end_render_pass(command_buffer);
    }

    // ssao pass
    {
        sb_image_transition transitions[] = {
            get_depth_readonly_transition(depth_buffer),
            get_fragment_attachment_transition(ssao_texture),
        };
        sb_set_image_layouts(command_buffer, transitions, COUNTOF(transitions));

        VkRenderingAttachmentInfo ssao_attachment = sb_rendering_attachment_info(ssao_texture, (sb_clear_value) {0});

        sb_render_pass_info ssao_pass = {0};
        ssao_pass.render_area = ssao_texture->extent;
        ssao_pass.color_attachment_count = 1;
        ssao_pass.color_attachments = &ssao_attachment;

        sb_begin_render_pass(command_buffer, &ssao_pass);

        sb_bind_graphics_pipeline(command_buffer, resources->ssao_pipeline);
        sb_draw_pixels(command_buffer);

        sb_end_render_pass(command_buffer);
    }   

    //ssao blur
    {
        sb_image_transition transitions[] = {
            get_fragment_attachment_transition(ssao_blur),
            get_fragment_readonly_transition(ssao_texture),
        };
        sb_set_image_layouts(command_buffer, transitions, COUNTOF(transitions));

        VkRenderingAttachmentInfo ssao_blur_attachment = sb_rendering_attachment_info(ssao_blur, (sb_clear_value) {0});

        sb_render_pass_info ssao_blur_pass = {0};
        ssao_blur_pass.render_area = ssao_blur->extent;
        ssao_blur_pass.color_attachment_count = 1;
        ssao_blur_pass.color_attachments = &ssao_blur_attachment;

        sb_begin_render_pass(command_buffer, &ssao_blur_pass);

        sb_bind_graphics_pipeline(command_buffer, resources->ssao_blur_pipeline);
        sb_draw_pixels(command_buffer);

        sb_end_render_pass(command_buffer);
    }

    // lighting pass
    {

        sb_image_transition swapchain_attachment_transition = {0};
        swapchain_attachment_transition.texture = swapchain_texture;
        swapchain_attachment_transition.new_layout = SB_IMAGE_LAYOUT_RENDER_ATTACHMENT;
        swapchain_attachment_transition.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        swapchain_attachment_transition.dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        sb_image_transition transitions[] = {
            get_depth_readonly_transition(shadow_map),
            swapchain_attachment_transition,

            get_fragment_readonly_transition(ssao_blur),
            get_fragment_readonly_transition(normal_texture),
            get_fragment_readonly_transition(albedoRGB_specularityA_texture),
        };

        sb_set_image_layouts(command_buffer, transitions, COUNTOF(transitions));

        VkRenderingAttachmentInfo swapchain_attachment = sb_rendering_attachment_info(swapchain_texture, (sb_clear_value) {0});

        sb_render_pass_info lighting_pass = {0};
        lighting_pass.color_attachment_count = 1;
        lighting_pass.color_attachments = &swapchain_attachment;
        lighting_pass.render_area = swapchain_texture->extent;

        sb_begin_render_pass(command_buffer, &lighting_pass);

        sb_bind_graphics_pipeline(command_buffer, resources->lighting_pipeline);
        sb_draw_pixels(command_buffer);

        sb_end_render_pass(command_buffer);

        sb_image_transition swapchain_texture_present_transition = {0};
        swapchain_texture_present_transition.texture = swapchain_texture;
        swapchain_texture_present_transition.old_layout = SB_IMAGE_LAYOUT_RENDER_ATTACHMENT;
        swapchain_texture_present_transition.new_layout = SB_IMAGE_LAYOUT_PRESENT;
        swapchain_texture_present_transition.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        swapchain_texture_present_transition.dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        sb_set_image_layouts(command_buffer, &swapchain_texture_present_transition, 1);
    }
}


sb_vec3 get_new_camera_position(const game_state_t *game_state, sb_vec3 camera_position, sb_vec3 player_position, float dt)
{
    sb_vec3 target_pos = sb_vec3_add(player_position, (sb_vec3){0,CUBE_SIDE_LENGTH*4.0 + game_state->scroll_pos ,0});
    return sb_vec3_lerp(camera_position, target_pos, dt*5.0);
}

typedef struct
{
  sb_texture_id ssao_id;
  sb_texture_id depth_buffer_id;
  sb_texture_id normal_id;
  sb_texture_id albedoRGB_specularityA_id;

  sb_texture_id shadow_map_id;
  uint32_t pad0[3];

  sb_vec4 sun_direction;
  sb_mat4 scene_view_to_shadow_light_space_matrix;
} lighting_ubo_t;

typedef struct 
{
    sb_mat4 projection;
    sb_mat4 view;
} camera_ubo_t;

#define SAMPLE_COUNT 128
typedef struct
{
    sb_texture_id depth_buffer_id;
    uint32_t pad[3];
    sb_vec4 offsets[SAMPLE_COUNT];
} ssao_ubo_t;

int main(void)
{
    sb_app_info app_info = {0};
    app_info.name = "Icebreaker";
    app_info.window_height = 1200;
    app_info.window_width = 900;
    app_info.bake_command_buffer = bake_command_buffer;
    sb_app *app = sb_create_app(&app_info);

    VkDeviceAddress time_address              = 0;
    VkDeviceAddress scene_camera_ubo_address  = 0;
    VkDeviceAddress shadow_camera_ubo_address = 0;
    VkDeviceAddress ssao_ubo_address          = 0;
    VkDeviceAddress ssao_blur_address         = 0;
    VkDeviceAddress lighting_ubo_address      = 0;

    float          *time              = sb_alloc_ubo(app, sizeof(float),          &time_address);
    camera_ubo_t   *scene_camera_ubo  = sb_alloc_ubo(app, sizeof(camera_ubo_t),   &scene_camera_ubo_address);
    camera_ubo_t   *shadow_camera_ubo = sb_alloc_ubo(app, sizeof(camera_ubo_t),   &shadow_camera_ubo_address);
    ssao_ubo_t     *ssao_ubo          = sb_alloc_ubo(app, sizeof(ssao_ubo_t),     &ssao_ubo_address);
    sb_texture_id  *ssao_id           = sb_alloc_ubo(app, sizeof(sb_texture_id),  &ssao_blur_address);
    lighting_ubo_t *lighting_ubo      = sb_alloc_ubo(app, sizeof(lighting_ubo_t), &lighting_ubo_address);

    sb_mat4_projection_ortho(shadow_camera_ubo->projection,  -25,  25,  -25,  25, 1,25);

    sb_vec3 scene_camera_position = {0};

    sb_mat4 scene_camera_rotation_matrix;
    sb_mat4_from_angles(scene_camera_rotation_matrix, (sb_vec3) {sb_rad(90), 0,sb_rad(90) });

    sb_vec3 sun_offset = {-3, 8, -3};
    sb_vec4 normalized_sun_offset = sb_make_vec3_vec4(sb_vec3_normalize(sun_offset), 1);

    sb_mat4 scene_camera_rotation_inverse;
    sb_mat4_transpose_to(scene_camera_rotation_matrix, scene_camera_rotation_inverse);
    lighting_ubo->sun_direction = sb_mat4_mul_vec4(scene_camera_rotation_inverse, normalized_sun_offset);

    resources_t resources = {0};

    // gpass resources
    {
        sb_texture_info depth_buffer_info = {0};
        depth_buffer_info.format = VK_FORMAT_D32_SFLOAT;
        depth_buffer_info.texture_type = SB_TEXTURE_TYPE_DEPTH;
        depth_buffer_info.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        depth_buffer_info.sampler_border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        depth_buffer_info.usage = SB_TEXTURE_USAGE_RENDER_ATTACHMENT_FLAG | SB_TEXTURE_USAGE_SHADER_READ_FLAG;
        depth_buffer_info.flags = SB_TEXTURE_FLAG_DEDICATED_ALLOCATION | SB_TEXTURE_FLAG_WINDOW_RELATIVE;

        resources.depth_buffer_id = sb_create_texture(app, &depth_buffer_info);

        sb_texture_info gbuffer_texture_info = {0};
        gbuffer_texture_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        gbuffer_texture_info.texture_type = SB_TEXTURE_TYPE_COLOR;
        gbuffer_texture_info.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        gbuffer_texture_info.sampler_border_color = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        gbuffer_texture_info.usage = SB_TEXTURE_USAGE_RENDER_ATTACHMENT_FLAG | SB_TEXTURE_USAGE_SHADER_READ_FLAG;
        gbuffer_texture_info.flags = SB_TEXTURE_FLAG_DEDICATED_ALLOCATION | SB_TEXTURE_FLAG_WINDOW_RELATIVE;
        resources.normal_texture_id = sb_create_texture(app, &gbuffer_texture_info);

        gbuffer_texture_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        resources.albedoRGB_specularityA_id = sb_create_texture(app, &gbuffer_texture_info);
    }

    // gpass pipeline
    {
        VkDeviceAddress gpass_vertex_shader_ubos[2] = {0};
        gpass_vertex_shader_ubos[0] = time_address;
        gpass_vertex_shader_ubos[1] = scene_camera_ubo_address;

        VkFormat gpass_attachments[2] = {0};
        gpass_attachments[0] = VK_FORMAT_R16G16B16A16_SFLOAT; // Normals
        gpass_attachments[1] = VK_FORMAT_R8G8B8A8_UNORM; // Albedo RGB & Specular A

        sb_graphics_pipeline_info gpass_pipeline = {0};
        gpass_pipeline.color_attachment_count = COUNTOF(gpass_attachments);
        gpass_pipeline.color_attachment_formats = gpass_attachments;
        gpass_pipeline.depth_attachment_format = VK_FORMAT_D32_SFLOAT;
        gpass_pipeline.vertex_shader_name = "shaders/spv/gpass.spv";
        gpass_pipeline.fragment_shader_name = "shaders/spv/gpass_pixel.spv";
        gpass_pipeline.cull_enabled = true;
        gpass_pipeline.vertex_ubo_count = COUNTOF(gpass_vertex_shader_ubos);
        gpass_pipeline.vertex_ubos = gpass_vertex_shader_ubos;

        resources.gpass_pipeline = sb_create_graphics_pipeline(app, &gpass_pipeline);
    }

    // shadow pass resources
    {
        sb_texture_info shadow_map_info = {0};
        shadow_map_info.format = VK_FORMAT_D32_SFLOAT; // TODO: 24 bit
        shadow_map_info.extent = (VkExtent2D) {2048,2048 };
        shadow_map_info.texture_type = SB_TEXTURE_TYPE_DEPTH;
        shadow_map_info.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        shadow_map_info.sampler_border_color = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        shadow_map_info.usage = SB_TEXTURE_USAGE_RENDER_ATTACHMENT_FLAG | SB_TEXTURE_USAGE_SHADER_READ_FLAG;
        shadow_map_info.flags = SB_TEXTURE_FLAG_DEDICATED_ALLOCATION;
        resources.shadow_map_id = sb_create_texture(app, &shadow_map_info);
    }

    // shadow pass pipeline
    {
        sb_depth_bias depth_bias = {0};
        depth_bias.constant_factor = 1.25f;
        depth_bias.slope_factor = 1.9f;

        VkDeviceAddress shadow_ubos[2] = {0};
        shadow_ubos[0] = shadow_camera_ubo_address;
        shadow_ubos[1] = time_address;

        sb_graphics_pipeline_info shadow_pipeline_info = {0};
        shadow_pipeline_info.depth_attachment_format = VK_FORMAT_D32_SFLOAT;
        shadow_pipeline_info.depth_bias = &depth_bias;
        shadow_pipeline_info.vertex_shader_name = "shaders/spv/shadow.spv";
        shadow_pipeline_info.cull_enabled = false;
        shadow_pipeline_info.vertex_ubo_count = COUNTOF(shadow_ubos);
        shadow_pipeline_info.vertex_ubos = shadow_ubos;

        resources.shadow_pipeline = sb_create_graphics_pipeline(app, &shadow_pipeline_info);
    }
     
    // ssao resources
    {
        sb_texture_info ssao_texture_info = {0};
        ssao_texture_info.format = VK_FORMAT_R8_UNORM;
        ssao_texture_info.texture_type = SB_TEXTURE_TYPE_COLOR;
        ssao_texture_info.usage = SB_TEXTURE_USAGE_RENDER_ATTACHMENT_FLAG | SB_TEXTURE_USAGE_SHADER_READ_FLAG;
        ssao_texture_info.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ssao_texture_info.sampler_border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        ssao_texture_info.flags = SB_TEXTURE_FLAG_DEDICATED_ALLOCATION | SB_TEXTURE_FLAG_WINDOW_RELATIVE;

        resources.ssao_id = sb_create_texture(app, &ssao_texture_info);
    }

    {
        ssao_ubo->depth_buffer_id = resources.depth_buffer_id;
        //generate samples
        for(int i = 0; i < SAMPLE_COUNT; i++)
        {
            float scale = i / (float)(SAMPLE_COUNT);

            //ignore W
            sb_vec4 offset = {sb_random_float(), sb_random_float(), sb_random_float(), 0};
            offset = sb_vec4_mul_f32(offset, 0.1f + 0.9f * scale * scale); // distributes more points closer to the origin

            ssao_ubo->offsets[i] = offset;
        }

        VkDeviceAddress ssao_ubos[2] = {0};
        ssao_ubos[0] = ssao_ubo_address;
        ssao_ubos[1] = scene_camera_ubo_address;

        VkFormat format = VK_FORMAT_R8_UNORM;

        sb_graphics_pipeline_info ssao_pipeline_info = {0};
        ssao_pipeline_info.color_attachment_count = 1;
        ssao_pipeline_info.color_attachment_formats = &format;
        ssao_pipeline_info.fragment_shader_name = "shaders/spv/ssao.spv";
        ssao_pipeline_info.fragment_ubos_count = COUNTOF(ssao_ubos);
        ssao_pipeline_info.fragment_ubos = ssao_ubos;
        resources.ssao_pipeline = sb_create_graphics_pipeline(app, &ssao_pipeline_info);
    }

    // ssao blur resources
    {
        sb_texture_info ssao_blur_texture_info = {0};
        ssao_blur_texture_info.format = VK_FORMAT_R8_UNORM;
        ssao_blur_texture_info.texture_type = SB_TEXTURE_TYPE_COLOR;
        ssao_blur_texture_info.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ssao_blur_texture_info.sampler_border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        ssao_blur_texture_info.usage = SB_TEXTURE_USAGE_RENDER_ATTACHMENT_FLAG | SB_TEXTURE_USAGE_SHADER_READ_FLAG;
        ssao_blur_texture_info.flags = SB_TEXTURE_FLAG_DEDICATED_ALLOCATION | SB_TEXTURE_FLAG_WINDOW_RELATIVE;

        resources.ssao_blur_id = sb_create_texture(app, &ssao_blur_texture_info);
    }

    /// ssao blur pipeline
    {
        *ssao_id = resources.ssao_id;
        VkDeviceAddress ssao_blur_ubos[1] = {0};
        ssao_blur_ubos[0] = ssao_blur_address;

        VkFormat format = VK_FORMAT_R8_UNORM;

        sb_graphics_pipeline_info ssao_blur_pipeline_info = {0};
        ssao_blur_pipeline_info.color_attachment_count = 1;
        ssao_blur_pipeline_info.color_attachment_formats = &format;
        ssao_blur_pipeline_info.fragment_shader_name = "shaders/spv/ssao_blur.spv";
        ssao_blur_pipeline_info.fragment_ubos_count = COUNTOF(ssao_blur_ubos);
        ssao_blur_pipeline_info.fragment_ubos = ssao_blur_ubos;
        resources.ssao_blur_pipeline = sb_create_graphics_pipeline(app, &ssao_blur_pipeline_info);
    }

    // lighting pass pipeline
    {
        lighting_ubo->ssao_id = resources.ssao_blur_id;
        lighting_ubo->depth_buffer_id = resources.depth_buffer_id;
        lighting_ubo->normal_id = resources.normal_texture_id;
        lighting_ubo->albedoRGB_specularityA_id = resources.albedoRGB_specularityA_id;
        lighting_ubo->shadow_map_id = resources.shadow_map_id;

        VkDeviceAddress lighting_ubos[2] = {0};
        lighting_ubos[0] = lighting_ubo_address;
        lighting_ubos[1] = scene_camera_ubo_address;

        sb_graphics_pipeline_info lighting_pass_pipeline_info = {0};
        lighting_pass_pipeline_info.color_attachment_count = 1;
        lighting_pass_pipeline_info.color_attachment_formats = &app->swapchain_image_format;
        lighting_pass_pipeline_info.fragment_shader_name = "shaders/spv/lighting.spv";
        lighting_pass_pipeline_info.fragment_ubos_count = COUNTOF(lighting_ubos);
        lighting_pass_pipeline_info.fragment_ubos = lighting_ubos;
        resources.lighting_pipeline = sb_create_graphics_pipeline(app, &lighting_pass_pipeline_info);
    }

    app->user_data = &resources;

    assets_t assets = load_assets(app);

    game_state_t game_state = {0};
    game_state.arena = sb_arena_alloc();
    new_level(&game_state);

    sb_window_event window_event;
    while(sb_run_app(app, &window_event))
    {
        if(window_event.flags & SB_WINDOW_RESIZED_FLAG)
            sb_mat4_projection_perpective(scene_camera_ubo->projection, sb_get_aspect_ratio(app->window), sb_rad(60),2.5f, 28.0f);


        *time = clock() / (float) CLOCKS_PER_SEC;

        float dt = sb_get_dt(app->window);

        move_tiles(game_state.level, dt);
        handle_input(&window_event, &game_state);
        handle_player_state(&game_state, dt);
        update_pillars(game_state.level, dt);
        update_teleport_colors(game_state.level, dt);
        rotate_keys(game_state.level, dt);
        draw_level(app, &assets, game_state.level);
    
        /// camera view updates
        sb_vec3 player_position = sb_mat4_get_position(get_player_tile(&game_state)->transform);
        scene_camera_position = get_new_camera_position(&game_state, scene_camera_position, player_position, dt);
        
        sb_mat4_look_at(sb_vec3_add(sun_offset, player_position), player_position, (sb_vec3) {0,1,0}, shadow_camera_ubo->view);

        sb_mat4 inverse_translation_matrix;
        sb_mat4_from_position(inverse_translation_matrix, sb_vec3_mul_f32(scene_camera_position, -1));
        sb_mat4_mul_mat4(scene_camera_rotation_inverse, inverse_translation_matrix, scene_camera_ubo->view);

        sb_mat4 scene_transform;
        sb_mat4_copy(scene_camera_rotation_matrix, scene_transform);
        sb_mat4_translate(scene_transform, scene_camera_position);

        sb_mat4 world_to_shadow_view;
        sb_mat4_mul_mat4(shadow_camera_ubo->view, scene_transform, world_to_shadow_view);

        sb_mat4 shadow_view_to_light_space;
        sb_mat4_mul_mat4(shadow_camera_ubo->projection, world_to_shadow_view, shadow_view_to_light_space);

        // This matrix moves the XY into screen space [0,1] from NDC [-1,1} so we can sample from the shadow map
        sb_mat4 shadow_pos_offset_matrix ={
            {0.5, 0.0, 0.0, 0.0},
            {0.0, 0.5, 0.0, 0.0},
            {0.0, 0.0, 1.0, 0.0},
            {0.5, 0.5, 0.0, 1.0},
        };

        sb_mat4_mul_mat4(shadow_pos_offset_matrix, shadow_view_to_light_space, lighting_ubo->scene_view_to_shadow_light_space_matrix);
    }

    return 0;
}
