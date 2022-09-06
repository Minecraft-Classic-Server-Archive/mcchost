#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

#include "createmap.h"

/*
 * This populates a default map file properties.

TODO:
    Default size from blocks file (move map_len_t right to end of file)
	--> only works if no blockdefs
 */

void
createmap(char * levelname)
{
    // Don't recreate a map that seems okay.
    if (level_prop->version_no == MAP_VERSION)
	if (level_prop->cells_x != 0 && level_prop->cells_y != 0 && level_prop->cells_z != 0)
	    return;

    // Save away the old size.
    xyzhv_t oldsize = {0};
    if (level_prop->magic_no == MAP_MAGIC) {
	if (level_prop->cells_x != 0 && level_prop->cells_y != 0 && level_prop->cells_z != 0)
	    if (level_prop->total_blocks == (int64_t)level_prop->cells_x * level_prop->cells_y * level_prop->cells_z)
	    {
		oldsize.x = level_prop->cells_x;
		oldsize.y = level_prop->cells_y;
		oldsize.z = level_prop->cells_z;
		oldsize.valid = 1;
	    }
    }

    init_map_null();

    level_prop->time_created = time(0),

    load_ini_file(level_ini_fields, MODEL_INI_NAME, 1, 0);

    patch_map_nulls(oldsize);

    open_blocks(levelname);

    map_len_t test_map;
    memcpy(&test_map, (void*)(level_blocks+level_prop->total_blocks),
	    sizeof(map_len_t));

    int blocks_valid = 1;
    if (test_map.magic_no != MAP_MAGIC) blocks_valid = 0;
    if (test_map.cells_x != level_prop->cells_x) blocks_valid = 0;
    if (test_map.cells_y != level_prop->cells_y) blocks_valid = 0;
    if (test_map.cells_z != level_prop->cells_z) blocks_valid = 0;

    if (!blocks_valid)
	init_flat_level();
}

void
init_map_null()
{
    *level_prop = (map_info_t){
	    .magic_no = MAP_MAGIC, .magic_no2 = MAP_MAGIC2,
	    .version_no = MAP_VERSION,
	    .cells_x = 0, .cells_y = 0, .cells_z = 0,
	    .total_blocks = 0,
	    .weather = 0, -1, -1, -1, -1, -1, -1,
	    .side_block = 7, 8, INT_MIN, -2,
	    .spawn = { INT_MIN, INT_MIN, INT_MIN },
	    .clouds_height = INT_MIN,
	    .clouds_speed = 256, 256, 128,
	    .click_distance = 160,
	    .hacks_jump = -1,
	};

    for(int i = 0; i<sizeof(level_prop->uuid); i++)
    {
#ifdef PCG32_INITIALIZER
        int by = pcg32_boundedrand(256);
#else
        int by = random() % 256;
#endif
	level_prop->uuid[i] = by;
    }
    // Not documented, but make it the bytes for a real UUID (big endian).
    level_prop->uuid[6] &= 0x0F;
    level_prop->uuid[6] |= 0x40;
    level_prop->uuid[10] &= 0x3F;
    level_prop->uuid[10] |= 0x80;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    memcpy(level_prop->blockdef, default_blocks, sizeof(default_blocks));
#pragma GCC diagnostic pop

    for (int i = 0; i<BLOCKMAX; i++) {
	level_prop->blockdef[i].inventory_order = i;
	level_prop->blockdef[i].fallback = i<CPELIMIT?i:Block_Orange;
    }

    for (int i = 0; i<16; i++)
	level_prop->blockdef[Block_CP+i].fallback = cpe_conversion[i];
}

void
patch_map_nulls(xyzhv_t oldsize)
{
    if (level_prop->cells_x >= 16384 || level_prop->cells_y >= 16384 || level_prop->cells_z >= 16384 ||
        level_prop->cells_x == 0 || level_prop->cells_y == 0 || level_prop->cells_z == 0 )
    {
	if (oldsize.valid == 1) {
	    level_prop->cells_x = oldsize.x;
	    level_prop->cells_y = oldsize.y;
	    level_prop->cells_z = oldsize.z;
	} else
	    level_prop->cells_y = (level_prop->cells_x = level_prop->cells_z = 128)/2;
    }

    level_prop->total_blocks = (int64_t)level_prop->cells_x * level_prop->cells_y * level_prop->cells_z;

    if (level_prop->side_level == INT_MIN)
	level_prop->side_level = level_prop->cells_y/2;

    if (level_prop->clouds_height == INT_MIN)
	level_prop->clouds_height = level_prop->cells_y+2;

    if (level_prop->spawn.x == INT_MIN)
	level_prop->spawn.x = level_prop->cells_x/2   *32+16;

    if (level_prop->spawn.y == INT_MIN)
	level_prop->spawn.y = level_prop->cells_y*3/4 *32+16;

    if (level_prop->spawn.z == INT_MIN)
	level_prop->spawn.z = level_prop->cells_z/2   *32+16;
}

void
init_flat_level()
{
    map_len_t test_map;
    int x, y, z, y1;
    struct timeval start, now;
    gettimeofday(&start, 0);

    if (strcasecmp(level_prop->theme, "pixel") == 0) {
	level_prop->side_level = 0;
	level_prop->seed[0] = 0;
	for(y=0; y<level_prop->cells_y; y++)
	    for(z=0; z<level_prop->cells_z; z++)
		for(x=0; x<level_prop->cells_x; x++)
		{
		    block_t px = Block_Air;
		    if (y==0) px = Block_Bedrock;
		    else if (x == 0 || z == 0) px = Block_White;
		    else if (x == level_prop->cells_x-1) px = Block_White;
		    else if (z == level_prop->cells_z-1) px = Block_White;
		    level_blocks[World_Pack(x,y,z)] = px;
		}

    } else if (strcasecmp(level_prop->theme, "empty") == 0) {
	level_prop->side_level = 1;
	level_prop->seed[0] = 0;
	for(y=0; y<level_prop->cells_y; y++)
	    for(z=0; z<level_prop->cells_z; z++)
		for(x=0; x<level_prop->cells_x; x++)
		{
		    block_t px = Block_Air;
		    if (y==0) px = Block_Bedrock;
		    level_blocks[World_Pack(x,y,z)] = px;
		}

    } else if (strcasecmp(level_prop->theme, "air") == 0) {
	level_prop->seed[0] = 0;
	for(y=0; y<level_prop->cells_y; y++)
	    for(z=0; z<level_prop->cells_z; z++)
		for(x=0; x<level_prop->cells_x; x++)
		    level_blocks[World_Pack(x,y,z)] = Block_Air;

    } else if (strcasecmp(level_prop->theme, "rainbow") == 0) {
	int has_seed = !!level_prop->seed[0];
	uint32_t seed;
	if (has_seed) seed = strtol(level_prop->seed, 0, 0);
	else {init_rand_gen(); seed = random();}
	level_prop->side_level = 1;
	for(y=0; y<level_prop->cells_y; y++)
	    for(z=0; z<level_prop->cells_z; z++)
		for(x=0; x<level_prop->cells_x; x++)
		{
		    if (y == 0 || x == 0 || x == level_prop->cells_x-1 ||
				  z == 0 || z == level_prop->cells_z-1) {
			block_t px = lehmer_pm(seed)%(Block_White-Block_Red)+Block_Red;
			level_blocks[World_Pack(x,y,z)] = px;
		    } else {
			level_blocks[World_Pack(x,y,z)] = Block_Air;
		    }
		}
	level_prop->dirty_save = !has_seed;

    } else if (strcasecmp(level_prop->theme, "space") == 0) {
	int has_seed = !!level_prop->seed[0];
	uint32_t seed;
	if (has_seed) seed = strtol(level_prop->seed, 0, 0);
	else {init_rand_gen(); seed = random();}
	level_prop->side_level = 1;
	level_prop->edge_block = Block_Obsidian;
	level_prop->sky_colour = 0x000000;
	level_prop->cloud_colour = 0x000000;
	level_prop->fog_colour = 0x000000;
	level_prop->ambient_colour = 0x9B9B9B;
	level_prop->sunlight_colour = 0xFFFFFF;
	for(y=0; y<level_prop->cells_y; y++)
	    for(z=0; z<level_prop->cells_z; z++)
		for(x=0; x<level_prop->cells_x; x++)
		{
		    block_t px = Block_Air;
		    if (y==0) px = Block_Bedrock;
		    else if (y==1 || y == level_prop->cells_y-1 ||
			     x == 0 || x == level_prop->cells_x-1 ||
			     z == 0 || z == level_prop->cells_z-1) {
			if (lehmer_pm(seed)%100 == 1)
			    px = Block_Iron;
			else
			    px = Block_Obsidian;
		    }
		    level_blocks[World_Pack(x,y,z)] = px;
		}
	level_prop->dirty_save = !has_seed;

    } else if (strcasecmp(level_prop->theme, "plain") == 0) {
	int has_seed = !!level_prop->seed[0];
	map_random_t rng[1];
	map_init_rng(rng, level_prop->seed);
	gen_plain_map(rng);
	level_prop->dirty_save = !has_seed;

    } else {
	if (strcasecmp(level_prop->theme, "flat") == 0) {
	    if (level_prop->seed[0])
		level_prop->side_level = atoi(level_prop->seed);
	} else {
	    strcpy(level_prop->theme, "Flat");
	    level_prop->seed[0] = 0;
	}

	y1 = level_prop->side_level-1;
	for(y=0; y<level_prop->cells_y; y++)
	    for(z=0; z<level_prop->cells_z; z++)
		for(x=0; x<level_prop->cells_x; x++)
		{
		    if (y>y1)
			level_blocks[World_Pack(x,y,z)] = 0;
		    else if (y==y1)
			level_blocks[World_Pack(x,y,z)] = Block_Grass;
		    else
			level_blocks[World_Pack(x,y,z)] = Block_Dirt;
		}
    }

    gettimeofday(&now, 0);
    printlog("Map gen (%d,%d,%d) theme=%s%s%s, time %.2fms",
	level_prop->cells_x, level_prop->cells_y, level_prop->cells_z,
	level_prop->theme, level_prop->seed[0]?", seed=":"", level_prop->seed,
	(now.tv_sec-start.tv_sec)*1000.0+(now.tv_usec-start.tv_usec)/1000.0);

    test_map.magic_no = MAP_MAGIC;
    test_map.cells_x = level_prop->cells_x;
    test_map.cells_y = level_prop->cells_y;
    test_map.cells_z = level_prop->cells_z;
    memcpy((void*)(level_blocks+level_prop->total_blocks),
	    &test_map, sizeof(map_len_t));
}
