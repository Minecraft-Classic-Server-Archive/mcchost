#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>

#include "createmap.h"

/*
 * This populates a default map file properties.

TODO:
    File source order -- Only one
	Uncompressed props file -- on open map.
	map/levelname.cw
	model.cw
	model.ini

    Default size from blocks file (move map_len_t right to end of file)
	--> only works if no blockdefs

 */

void
createmap(char * levelname)
{
    if (level_prop->version_no == MAP_VERSION)
	if (level_prop->cells_x != 0 && level_prop->cells_y != 0 && level_prop->cells_z != 0)
	    return;

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

    char buf2[256];
    snprintf(buf2, sizeof(buf2), "map/%.200s.ini", levelname);
    load_ini_file(level_ini_fields, buf2, 1);

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

    // If level not valid wipe to flat.
    if (!blocks_valid) {
        int x, y, z, y1;

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

	test_map.magic_no = MAP_MAGIC;
	test_map.cells_x = level_prop->cells_x;
	test_map.cells_y = level_prop->cells_y;
	test_map.cells_z = level_prop->cells_z;
	memcpy((void*)(level_blocks+level_prop->total_blocks),
		&test_map, sizeof(map_len_t));
    }
}

void
init_map_from_size(xyz_t size)
{
    *level_prop = (map_info_t){
	    .magic_no = MAP_MAGIC, .magic_no2 = MAP_MAGIC2,
	    .version_no = MAP_VERSION,
	    .cells_x = size.x, .cells_y = size.y, .cells_z = size.z,
	    .total_blocks = (int64_t)size.x*size.y*size.z,
	    .weather = 0, -1, -1, -1, -1, -1, -1,
	    .side_block = 7, 8, size.y/2, -2,
	    .spawn = { size.x*32+16, size.y*3/4*32, size.z*32+16 },
	    .clouds_height = size.y+2,
	    .clouds_speed = 256, 256, 128,
	    .skybox_hor_speed = 1024, 1024,
	    .click_distance = -1
	};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    memcpy(level_prop->blockdef, default_blocks, sizeof(default_blocks));
#pragma GCC diagnostic pop

    for (int i = 0; i<BLOCKMAX; i++)
	level_prop->blockdef[i].inventory_order = i;
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
	    .skybox_hor_speed = 1024, 1024,
	    .click_distance = -1
	};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    memcpy(level_prop->blockdef, default_blocks, sizeof(default_blocks));
#pragma GCC diagnostic pop

    for (int i = 0; i<BLOCKMAX; i++)
	level_prop->blockdef[i].inventory_order = i;
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
