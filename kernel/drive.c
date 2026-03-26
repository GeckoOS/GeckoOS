#include "drivers/drives/drive.h"
#include "drivers/drives/ata.h"
#include "drivers/drives/sata.h"
#include "terminal/terminal.h"
#include "mem.h"

static const struct kdmaster_t drive_masters[] = {
	{
		ata_master_init
	},
	{ 0 }
};

static struct kdrive_t drives[KMAX_DRIVES] = {

};

void kdrive_register( struct kdrive_t drive )
{
	int i;

	for (i = 0; i < KMAX_DRIVES; i++ )
	{
		if (drives[i].sector_size != 0)
			continue;
		drives[i] = drive;

		break;
	}
}

struct kdrive_t *get_drive( int i )
{
	if (drives[i].sector_size == 0)
		return 0;
	return &drives[i];
}

void drives_init()
{
	struct kdmaster_t master;
	int i;
	memset(drives, 0, sizeof(drives));

	i = 0;
	for (;;)
	{
		master = drive_masters[i];
		if ( master.init == 0 )
			break;

		master.init();

		i++;
	}
	print("Drives inited\n");
}
