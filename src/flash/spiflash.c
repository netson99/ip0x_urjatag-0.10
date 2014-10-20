/*
 * $Id: spiflash.c,v 1.2 2006/10/16 16:06:35 danov Exp $
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

#include <stdint.h>
#include <string.h>
#include <flash/cfi.h>
#include <flash/intel.h>
#include <flash/mic.h>

//#include <flash.h>
//#include <cfi.h>
//#include <bus.h>

#include "bus.h"
#include "flash.h"
#include "jtag.h"

// M25P64
#define	NUM_SECTORS 	128	/* number of sectors */
#define SECTOR_SIZE	0x10000
#define NOP_NUM		1000

//Flash commands
#define SPI_WREN	(0x06)  // Set Write Enable Latch
#define SPI_WRDI	(0x04)  // Reset Write Enable Latch
#define SPI_RDSR	(0x05)  // Read Status Register
#define SPI_WRSR	(0x01)  // Write Status Register
#define SPI_READ	(0x03)  // Read data from memory
#define SPI_PP		(0x02)  // Program Data into memory
#define SPI_SE		(0xD8)  // Erase one sector in memory
#define SPI_BE		(0xC7)  // Erase all memory
#define SPI_RES		(0xAB)  // Read Electonic Signature
#define SPI_RDID	(0x9f)  // Read Identification
#define WIP		(0x1)	// Check the write in progress bit of the SPI status register
#define WEL		(0x2)	// Check the write enable bit of the SPI status register
#define TIMEOUT		1000000


static uint8_t manuf = 0;
static uint8_t mem_type = 0;
static uint8_t mem_cap = 0;

static void spi_single_cmd (bus_t *bus, uint8_t cmd) {

	bus_spi_on (bus);
	bus_spi_send (bus, cmd);
	bus_spi_off (bus);
}

static void flash_rdid (bus_t *bus) {

	bus_spi_on (bus);
	bus_spi_send (bus, SPI_RDID);		// command
	manuf = bus_spi_recv (bus);		// Manufacturer ID
	mem_type = bus_spi_recv (bus);		// Device ID
	mem_cap = bus_spi_recv (bus);

	/* Many ST M25P20's dont support the RDID instruction so we forge
	   blindly ahead */
	if ((manuf == 0xff) && (mem_type == 0xff) && (mem_cap == 0xff)) {
	  printf("All 0xffs returned\n");
	  printf("Making the dangerous assumption that we have a M25P20!\n");
	  manuf = 0x20;
	  mem_type = 0x20;
	  mem_cap = 0x12;
	}
	bus_spi_off (bus);
}

#if 0
static uint8_t flash_res (bus_t *bus) {
uint8_t sr;

	bus_spi_on (bus);
	bus_spi_send (bus, SPI_RES);	// command
	bus_spi_send (bus, SPI_RES);	// command
	bus_spi_send (bus, SPI_RES);	// command
	bus_spi_send (bus, SPI_RES);	// command
	sr = bus_spi_recv (bus);	// status register
	bus_spi_off (bus);
	return sr;
}

static uint8_t flash_status (bus_t *bus) {
uint8_t sr;


	bus_spi_on (bus);
	bus_spi_send (bus, SPI_RDSR);	// command
	sr = bus_spi_recv (bus);	// status register
	bus_spi_off (bus);
	return sr;
}
#endif

static int flash_rdsr (bus_t *bus, int flag, int value, int timeout) {
int i, r = -1;
uint8_t sr;


	bus_spi_on (bus);
	bus_spi_send (bus, SPI_RDSR);			// command
	for (i = 0; i < timeout; i++) {
		sr = bus_spi_recv (bus);		// status register
		if (flag) {
			if ((sr & flag) == value) {
				r = 0;
				break;
			}
		}
	}
	bus_spi_off (bus);
	return r;
}

int spi_write_flash (bus_t *bus, uint32_t addr, uint32_t cnt,
				uint8_t *data, uint32_t *ocnt) {
uint32_t real_cnt = 0;
int i;

	*ocnt = 0;
	// 1. a Write Enable Command must be sent to the SPI.
	spi_single_cmd (bus, SPI_WREN);

	// 2. the SPI Status Register will be tested whether the
	//         Write Enable Bit has been set.
	if (flash_rdsr (bus, WEL, WEL, TIMEOUT)) return -1;

	// 3. the 24 bit address will be shifted out the SPI MOSI bytewise.
	bus_spi_on (bus);
	bus_spi_send (bus, SPI_PP);		// send page program
	bus_spi_send (bus, (addr >> 16) & 0xff);
	bus_spi_send (bus, (addr >> 8) & 0xff);
	bus_spi_send (bus, (addr >> 0) & 0xff);

	// 4. maximum number of 256 bytes will be taken from the Buffer
	// and sent to the SPI device.
	for (i = 0; (i < cnt) && (i < 256); i++, real_cnt++) {

		bus_spi_send (bus, *data);
		data ++;
	}
	bus_spi_off (bus);

	// 5. the SPI Write in Progress Bit must be toggled to ensure the
	// programming is done before start of next transfer.
	if (flash_rdsr (bus, WIP, 0, TIMEOUT)) return -1;

	*ocnt = real_cnt;
	return 0;
}

static int spi_write (bus_t *bus, uint32_t addr, uint32_t cnt, uint8_t *data) {
uint32_t rcnt;

	while (cnt) {
		if (spi_write_flash (bus, addr, cnt, data, & rcnt))
			return -1;

		// After each function call of spi_write_flash
		// the counter must be adjusted
		cnt -= rcnt;

		// Also, both address pointers must be recalculated.
		addr += rcnt;
		data += rcnt;
	}
	// return the appropriate error code
	return 0;
}

int spi_erase_block (bus_t *bus, int nblock) {
uint32_t addr;


	// if the block is invalid just return
	if ((nblock < 0) || (nblock > NUM_SECTORS)) {
		return -1;
	}

	addr = nblock * SECTOR_SIZE;

	// A write enable instruction must previously have been executed
	spi_single_cmd (bus, SPI_WREN);

	// The status register will be polled to check the write
	// enable latch "WREN"
	if (flash_rdsr (bus, WEL, WEL, TIMEOUT)) {
		printf ("SPI Erase block error: can't set WEL\n");
		return -1;
	}

	// Send the erase block command to the flash followed by the 24 address
	// to point to the start of a sector.
	bus_spi_on (bus);
	bus_spi_send (bus, SPI_SE);
	bus_spi_send (bus, (addr >> 16) & 0xff);
	bus_spi_send (bus, (addr >> 8) & 0xff);
	bus_spi_send (bus, (addr >> 0) & 0xff);

	//Turns off the SPI
	bus_spi_off (bus);

	// Poll the status register to check the Write in Progress bit
	// Sector erase takes time
	if (flash_rdsr (bus, WIP, 0, TIMEOUT)) return -1;

	// block erase should be complete
	return 0;
}


void spidetectflash (bus_t *bus, uint32_t adr) {
#if 0
uint8_t sr;
#endif

	manuf = mem_type = mem_cap = 0;
	bus_prepare (bus);
	flash_rdid (bus);
	printf ( _("SPI FLASH:\n"
		"\tManufacturer ID: %02X\n"
		"\tMemory type: %02X\n"
		"\tMemory capacity: %02X (%i bytes)\n"),
		manuf, mem_type, mem_cap, 1 << mem_cap);
	if ((manuf == 0x20) && (mem_type == 0x20) && (mem_cap == 0x16)) {
		printf ( _("\tChip: STMicroelectronics M25P32\n"));
	} else if ((manuf == 0x20) && (mem_type == 0x20) && (mem_cap == 0x17)) {
		printf ( _("\tChip: STMicroelectronics M25P64\n"));
	} else if ((manuf == 0x20) && (mem_type == 0x20) && (mem_cap == 0x12)) {
		printf ( _("\tChip: STMicroelectronics M25P20\n"));
	} else
		printf ( _("\tChip: Unknown\n"));
#if 0
	sr = flash_res (bus);
	printf ( _("\tRES: %02X\n"), sr);
	sr = flash_status (bus);
	printf ( _("\tSTATUS: \n"
		"\t\tStatus Register Write Protect: %i\n"
		"\t\tBP2: %i\n"
		"\t\tBP1: %i\n"
		"\t\tBP0: %i\n"
		"\t\tWEL: %i\n"
		"\t\tWIP: %i\n"),
		(sr & 0x80) ? 1 : 0,
		(sr & 0x10) ? 1 : 0,
		(sr & 0x08) ? 1 : 0,
		(sr & 0x04) ? 1 : 0,
		(sr & 0x02) ? 1 : 0,
		(sr & 0x01) ? 1 : 0);
#endif
}

void spireadflash (bus_t *bus, uint32_t addr, uint32_t len, FILE *f) {
uint32_t a;
int bc = 0;
#define BSIZE 256
uint8_t b[BSIZE];

	if ((! bus) || (! bus->driver->spi_on) || (! bus->driver->spi_off) ||
	(! bus->driver->spi_recv) || (! bus->driver->spi_send)) {
		printf ( _("spireadflash Error: Bus driver missing.\n"));
		return;
	}

	bus_prepare (bus);
	printf ( _("address: 0x%08X\n"), addr);
	printf ( _("length:  0x%08X\n"), len);

	if (len == 0) {
		printf ( _("length is 0.\n"));
		return;
	}

	printf ( _("reading:\n") );
	printf ( _("addr: 0x%08X"), addr);
	printf ("\r");
	fflush (stdout);

	bus_spi_on (bus);
	bus_spi_send (bus, SPI_READ);
	bus_spi_send (bus, (addr >> 16) & 0xff);
	bus_spi_send (bus, (addr >> 8) & 0xff);
	bus_spi_send (bus, (addr >> 0) & 0xff);

	for (a = addr; a < addr + len; a ++) {
		b [bc++] = bus_spi_recv (bus);

		if (bc < BSIZE) continue;
		printf ( _("addr: 0x%08X"), a + 1);
		printf ("\r");
		fflush (stdout);
		fwrite (b, bc, 1, f);
		bc = 0;
	}
	bus_spi_off (bus);
	if (bc) fwrite (b, bc, 1, f);
	printf( _("\nDone.\n") );
}

void spiflashmem (bus_t *bus, uint32_t addr, FILE *f) {
uint32_t a;
#define BSIZE 256
uint8_t b [BSIZE];
size_t nread;

	if ((! bus) || (! bus->driver->spi_on) || (! bus->driver->spi_off) ||
	(! bus->driver->spi_recv) || (! bus->driver->spi_send)) {
		printf ( _("spireadflash Error: Bus driver missing.\n"));
		return;
	}
	bus_prepare (bus);

	printf ( _("program:\n"));
	a = addr;
	for (;;) {
		printf ( _("addr: 0x%08X"), a);
		printf ( "\r");
		fflush (stdout);
		nread = fread (b, 1, BSIZE, f);
		if ((nread == 0) && (feof( f ) != 0))
			break;
		if ((nread < BSIZE) && (ferror( f ) != 0)) {
			printf ( _("\nFile read error!\n"));
			return;
		}
		if (spi_write (bus, a, nread, b)) {
			printf ( _("\nWrite error!\n"));
			return;
		}

		a += nread;
	}
	printf ( _("addr: 0x%08X (done)\n"), a);
	a = addr;

	fseek (f, 0, SEEK_SET);
	printf ( _("verify:\n"));
	fflush (stdout);

	printf ( _("addr: 0x%08X"), addr);
	printf ("\r");
	fflush (stdout);

	bus_spi_on (bus);
	bus_spi_send (bus, SPI_READ);
	bus_spi_send (bus, (addr >> 16) & 0xff);
	bus_spi_send (bus, (addr >> 8) & 0xff);
	bus_spi_send (bus, (addr >> 0) & 0xff);

	while (! feof (f)) {
	uint8_t x, y;


		if (fread (& x, 1, 1, f ) != 1) {
			if (feof(f)) break;
			printf( _("\nFile read error!\n") );
			return;
		}

		y = bus_spi_recv (bus);

		if (x != y) {
			printf( _("\nverify error: address 0x%08X, "
				"readed: 0x%08X, "
				"expected: 0x%08X\n"),
				a, y, x);
		} else {
			printf ( _("addr: 0x%08X"), a);
			printf ("\r");
			fflush (stdout);
		}
		a ++;
	}
	bus_spi_off (bus);
	printf( _("\nDone.\n") );
}

void spieraseflash (bus_t *bus, uint32_t addr, uint32_t blocks) {
uint32_t nblock, i;

	if ((! bus) || (! bus->driver->spi_on) || (! bus->driver->spi_off) ||
	(! bus->driver->spi_recv) || (! bus->driver->spi_send)) {
		printf ( _("spireadflash Error: Bus driver missing.\n"));
		return;
	}
	bus_prepare (bus);

	printf ( _("erasing:\n"));

	nblock = addr / SECTOR_SIZE;
	for (i = 0; i < blocks; i ++) {
		if (spi_erase_block (bus, i + nblock)) {
			printf ( _("spireadflash Error: "
				"can't erase block %i (0x%08X)\n"),
				i + nblock, (i + nblock) * SECTOR_SIZE);
			return;
		}
		printf ( _("addr: 0x%08X"), (i + nblock) * SECTOR_SIZE);
		printf ("\r");
		fflush (stdout);
	}
}
