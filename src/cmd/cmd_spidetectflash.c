/*
 * $Id: cmd_spidetectflash.c,v 1.2 2006/10/16 16:06:35 danov Exp $
 *
 * Copyright (C) 2006 Intratrade Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by Ivan Danov <idanov@gmail.com>, 2006.
 *
 */

#include "sysdep.h"

#include <stdio.h>

#include <flash.h>
#include <cmd.h>
extern bus_t *bus;
extern spidetectflash(bus_t *bus, uint32_t adr);

static int cmd_spieraseflash_run (chain_t *chain, char *params []) {
uint32_t addr, blocks;

	if (cmd_params (params) != 3)
		return -1;

	if ((! bus) || (! bus->driver->spi_on) || (! bus->driver->spi_off) ||
	(! bus->driver->spi_recv) || (! bus->driver->spi_send)) {
		printf( _("%s Error: Bus driver missing.\n"), params [0] );
		return 1;
	}

	if (cmd_get_number (params [1], & addr))
		return -1;
	if (cmd_get_number (params [2], & blocks))
		return -1;

	spieraseflash (bus, addr, blocks);
	return 1;
}

static int cmd_spiflashmem_run (chain_t *chain, char *params []) {
uint32_t addr;
FILE *f;

	if (cmd_params (params) != 3)
		return -1;

	if ((! bus) || (! bus->driver->spi_on) || (! bus->driver->spi_off) ||
	(! bus->driver->spi_recv) || (! bus->driver->spi_send)) {
		printf( _("%s Error: Bus driver missing.\n"), params [0] );
		return 1;
	}

	if (cmd_get_number (params [1], & addr))
		return -1;
	f = fopen (params [2], "r");
	if (!f) {
		printf( _("Unable to read file `%s'!\n"), params [2]);
		return -1;
	}
	spiflashmem (bus, addr, f);
	fclose (f);
	return 1;
}

static int cmd_spireadflash_run (chain_t *chain, char *params []) {
uint32_t addr, cnt;
FILE *f;

	if (cmd_params (params) != 4)
		return -1;

	if ((! bus) || (! bus->driver->spi_on) || (! bus->driver->spi_off) ||
	(! bus->driver->spi_recv) || (! bus->driver->spi_send)) {
		printf( _("%s Error: Bus driver missing.\n"), params [0] );
		return 1;
	}

	if (cmd_get_number (params [1], & addr))
		return -1;
	if (cmd_get_number (params [2], & cnt))
		return -1;

	f = fopen (params [3], "w");
	if (!f) {
		printf( _("Unable to create file `%s'!\n"), params [3]);
		return -1;
	}
	spireadflash (bus, addr, cnt, f);
	fclose (f);

	return 1;
}


static int cmd_spidetectflash_run (chain_t *chain, char *params []) {
uint32_t adr;
int r;

	if ((r = cmd_params (params)) != 2) {
		printf( _("spidetectflash Error: no params %d != 2.\n"), r );
		return -1;
	}

	if ((! bus) || (! bus->driver->spi_on) || (! bus->driver->spi_off) ||
	(! bus->driver->spi_recv) || (! bus->driver->spi_send)) {
		printf( _("%s Error: Bus driver missing.\n"), params [0] );
		return 1;
	}

	if (cmd_get_number (params [1], &adr)) {
		printf( _("%s Error:  get number params %s.\n"), params [0], params [1] );
		return -1;
	}

	spidetectflash (bus, adr);
	return 1;
}

static void cmd_spidetectflash_help (void)
{
	printf( _(
		"Usage: spidetectflash ADDRESS\n"
		"Detect SPI flash memory type connected to a part.\n"
		"\n"
		"ADDRESS    Base address for memory region\n"
	));
}

cmd_t cmd_spidetectflash = {
	"spidetectflash",
	N_("spidetect parameters of SPI flash chips attached to a part"),
	cmd_spidetectflash_help,
	cmd_spidetectflash_run
};


static void cmd_spieraseflash_help (void) {
	printf ( _("Usage: spieraseflash ADDR BLOCKS\n"
		"Erase SPI flash memory from ADDR.\n"
		"\n"
		"ADDR       target addres for erasing block\n"
		"BLOCKS     number of blocks to erase\n"
		"\n"
		"ADDR and BLOCKS could be in decimal or hexadecimal (prefixed with 0x) form.\n"
		"\n"
		"Supported Flash Memories:\n"
		"M25P64\n"));
}

static void cmd_spiflashmem_help (void) {

	printf ( _("Usage: spiflashmem ADDR FILENAME\n"
		"Program FILENAME content to SPI flash memory.\n"
		"\n"
		"ADDR       target address for raw binary image\n"
		"FILENAME   name of the input file\n"
		"\n"
		"ADDR could be in decimal or hexadecimal (prefixed with 0x) form.\n"
		"\n"
		"Supported Flash Memories:\n"
		"M25P64\n"));
}

static void cmd_spireadflash_help (void) {

	printf ( _("Usage: spireadflash ADDR LEN FILENAME\n"
		"Copy SPI flash memory content starting with ADDR to FILENAME file.\n"
		"\n"
		"ADDR       start address of the copied memory area\n"
		"LEN        copied memory length\n"
		"FILENAME   name of the output file\n"
		"\n"
		"ADDR and LEN could be in decimal or hexadecimal (prefixed with 0x) form.\n"
		"\n"
		"Supported Flash Memories:\n"
		"M25P64\n"));
}


cmd_t cmd_spieraseflash = {
	"spieraseflash",
	N_("erase SPI flash memory by number of blocks"),
	cmd_spieraseflash_help,
	cmd_spieraseflash_run
};

cmd_t cmd_spiflashmem = {
	"spiflashmem",
	N_("burn SPI flash memory with data from a file"),
	cmd_spiflashmem_help,
	cmd_spiflashmem_run
};

cmd_t cmd_spireadflash = {
	"spireadflash",
	N_("read content of the SPI flash and write it to file"),
	cmd_spireadflash_help,
	cmd_spireadflash_run
};

