#include <stdio.h>
#include <string.h>

#include "cmdwho.h"

/*HELP who H_CMD
&T/who
List out the connected users
*/

#if INTERFACE
#define CMD_WHO \
    {N"who", &cmd_who}, \
    {N"pclients", &cmd_clients}, {N"clients", &cmd_clients, .dup=1}
#endif

void
cmd_who(char * UNUSED(cmd), char * UNUSED(arg))
{
    int users = 0;

    for(int i=0; i<MAX_USER; i++)
    {
	client_entry_t c = shdat.client->user[i];
	if (i == my_user_no) continue;
	if (!c.active) continue;
	users++;

	char nbuf[256];
	snprintf(nbuf, sizeof(nbuf), "%s%s", c.name.c, c.is_afk?" (AFK)":"");

	int level_id = c.on_level;
	if (level_id<0)
	    printf_chat("\\%s is between levels", nbuf);
	else if (shdat.client->levels[level_id].backup_id>0)
	    printf_chat("\\%s is on museum %d of %s at (%d,%d,%d)",
		nbuf,
		shdat.client->levels[level_id].backup_id,
		shdat.client->levels[level_id].level.c,
		c.posn.x/32, c.posn.y/32, c.posn.z/32);
	else if (shdat.client->levels[level_id].backup_id==0)
	    printf_chat("\\%s is on %s at (%d,%d,%d)",
		nbuf, shdat.client->levels[level_id].level.c,
		c.posn.x/32, c.posn.y/32, c.posn.z/32);
	else
	    printf_chat("\\%s is in the void", nbuf);
    }

    if (current_level_backup_id == 0)
	printf_chat("You are on %s at (%d,%d,%d)",
	    current_level_name, player_posn.x/32, player_posn.y/32, player_posn.z/32);
    else if (current_level_backup_id>0)
	printf_chat("You are on museum %d of %s at (%d,%d,%d)",
	    current_level_backup_id, current_level_name,
	    player_posn.x/32, player_posn.y/32, player_posn.z/32);
    else
	printf_chat("You are nowhere.");

    if (users == 0)
	printf_chat("There are currently no other users");
}

void
cmd_clients(char * UNUSED(cmd), char * UNUSED(arg))
{
    char linebuf[BUFSIZ];
    printf_chat("Players using:");

    int users = 0;

    for(int i=0; i<MAX_USER; i++)
        shdat.client->user[i].client_dup = 0;

    for(int i=0; i<MAX_USER; i++) {
        if (shdat.client->user[i].active == 1) {
            users ++;
            if (shdat.client->user[i].client_dup == 0) {
                for(int j = i+1; j<MAX_USER; j++) {
                    if (shdat.client->user[j].active == 1 &&
                        strcmp(shdat.client->user[i].client_software.c,
                        shdat.client->user[j].client_software.c) == 0)
                    {
                        shdat.client->user[j].client_dup = 1;
                    }
                }
            }
        }
    }

    for(int j = 0; j<MAX_USER; j++)
    {
	client_entry_t c1 = shdat.client->user[j];
	if (!c1.active) continue;
	if (c1.client_dup) continue;

	int l = snprintf(linebuf, sizeof(linebuf), "%s: &f%s", c1.client_software.c, c1.name.c);
	for(int i=j; i<MAX_USER; i++)
	{
	    client_entry_t c = shdat.client->user[i];
	    if (!c.active || !c.client_dup) continue;
	    if (strcmp(c1.client_software.c, c.client_software.c) != 0)
		continue;

	    l += snprintf(linebuf+l, sizeof(linebuf)-l, ", %s", c.name.c);
	}

	printf_chat("  %s", linebuf);
    }

}
