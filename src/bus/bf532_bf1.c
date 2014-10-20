/*
 * $Id: bf532_bf1.c,v 1.1.1.1 2006/10/16 15:47:29 danov Exp $
 *
 * Copyright (C) 2002 ETC s.r.o.
 *           (C) 2006 Intratrade Ltd.
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
 * Written by Christian Pellegrin <chri@ascensit.com>, 2003.
 * Modified by Marcel Telka <marcel@telka.sk>, 2003.
 * Modified by Marcel Telka <marcel@telka.sk>, 2003.
 * Modified by Ivan Danov <idanov@gmail.com>, 2006
 * Modified by Thomas Triadi <netson99@gmail.com>, 2014 => rewrite for urjtag_0.10
 *
 */

#include "sysdep.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "part.h"
#include "bus.h"
#include "chain.h"
#include "bssignal.h"
#include "jtag.h"
#include "buses.h"
#include "generic_bus.h"

#define DLINE printf ( _("%s %s %i\n"), __FILE__, __FUNCTION__, __LINE__)

//#define DPRINTF printf
#define DPRINTF(...)

typedef struct {
	chain_t *chain;
	part_t *part;
	signal_t *pf2;
	signal_t *pf13;
	signal_t *ams[4];
	signal_t *addr[19];
	signal_t *data[16];
	signal_t *awe;
	signal_t *aoe;
	signal_t *sras;
	signal_t *scas;
	signal_t *sms;
	signal_t *swe;
	signal_t *miso;
	signal_t *mosi;
	signal_t *sck;
} bus_params_t;

//#define	CHAIN	((bus_params_t *) bus->params)->chain
//#define	PART	((bus_params_t *) bus->params)->part
#define	PF2	((bus_params_t *) bus->params)->pf2
#define	PF13	((bus_params_t *) bus->params)->pf13
#define	AMS	((bus_params_t *) bus->params)->ams
#define	ADDR	((bus_params_t *) bus->params)->addr
#define	DATA	((bus_params_t *) bus->params)->data
#define	AWE	((bus_params_t *) bus->params)->awe
#define	AOE	((bus_params_t *) bus->params)->aoe
#define	SRAS	((bus_params_t *) bus->params)->sras
#define	SCAS	((bus_params_t *) bus->params)->scas
#define	SMS	((bus_params_t *) bus->params)->sms
#define	SWE	((bus_params_t *) bus->params)->swe
#define	MISO	((bus_params_t *) bus->params)->miso
#define	MOSI	((bus_params_t *) bus->params)->mosi
#define	SCK	((bus_params_t *) bus->params)->sck

// SPI
static void bf532_bf1_bus_spi_on (bus_t *bus) {

	part_set_signal (PART, PF2, 1, 0);	// PF2 = 0
	part_set_signal (PART, MOSI, 1, 1);
	part_set_signal (PART, SCK, 1, 1);
	part_set_signal (PART, MISO, 0, 0);
	chain_shift_data_registers (CHAIN, 0);
	DPRINTF ("SPI on\n");
}

static void bf532_bf1_bus_spi_off (bus_t *bus) {

	part_set_signal (PART, PF2, 1, 1);	// PF2 = 1
	part_set_signal (PART, MOSI, 1, 1);
	part_set_signal (PART, SCK, 1, 1);
	part_set_signal (PART, MISO, 0, 0);
	chain_shift_data_registers (CHAIN, 0);
	DPRINTF ("SPI off\n");
}

static void bf532_bf1_bus_spi_send (bus_t *bus, uint8_t b) {
uint8_t c = 0x01, x;

	DPRINTF ("send %02X\n", b);
	while (c) {
		x = (b & 0x80) ? 1 : 0;
		part_set_signal (PART, MOSI, 1, x);
		part_set_signal (PART, SCK, 1, 0);
		chain_shift_data_registers (CHAIN, 0);
		part_set_signal (PART, SCK, 1, 1);
		chain_shift_data_registers (CHAIN, 0);
		b <<= 1;
		c <<= 1;
	}
	part_set_signal (PART, MOSI, 1, x);
}

static uint8_t bf532_bf1_bus_spi_recv (bus_t *bus) {
uint8_t b = 0, c = 0x01;

	while (c) {
		part_set_signal (PART, SCK, 1, 0);
		chain_shift_data_registers (CHAIN, 1);
		b |= part_get_signal (PART, MISO) & 0x01;
		part_set_signal (PART, SCK, 1, 1);
		chain_shift_data_registers (CHAIN, 1);
		c <<= 1;
		b <<= 1;
	}
	b |= part_get_signal (PART, MISO) & 0x01;
	DPRINTF ("recv %02X\n", b);
	return b;
}

static void
select_flash( bus_t *bus )
{
	part_t *p = PART;

	part_set_signal( p, PF2, 1, 1 );
	part_set_signal( p, PF13, 1, 0 );

	part_set_signal( p, AMS[0], 1, 0 );
	part_set_signal( p, AMS[1], 1, 1 );
	part_set_signal( p, AMS[2], 1, 1 );
	part_set_signal( p, AMS[3], 1, 1 );

	part_set_signal( p, SRAS, 1, 1 );
	part_set_signal( p, SCAS, 1, 1 );
	part_set_signal( p, SWE, 1, 1 );
	part_set_signal( p, SMS, 1, 1 );
}

static void
unselect_flash( bus_t *bus )
{
	part_t *p = PART;

	part_set_signal( p, PF2, 1, 1 );
	part_set_signal( p, PF13, 1, 0 );

	part_set_signal( p, AMS[0], 1, 1 );
	part_set_signal( p, AMS[1], 1, 1 );
	part_set_signal( p, AMS[2], 1, 1 );
	part_set_signal( p, AMS[3], 1, 1 );

	part_set_signal( p, SRAS, 1, 1 );
	part_set_signal( p, SCAS, 1, 1 );
	part_set_signal( p, SWE, 1, 1 );
	part_set_signal( p, SMS, 1, 1 );
}

static void
setup_address( bus_t *bus, uint32_t a )
{
	int i;
	part_t *p = PART;

	for (i = 0; i < 19; i++)
		part_set_signal( p, ADDR[i], 1, (a >> (i + 1)) & 1 );
}

static void
set_data_in( bus_t *bus )
{
	int i;
	part_t *p = PART;

	for (i = 0; i < 16; i++)
		part_set_signal( p, DATA[i], 0, 0 );
}

static void
setup_data( bus_t *bus, uint32_t d )
{
	int i;
	part_t *p = PART;

	for (i = 0; i < 16; i++)
		part_set_signal( p, DATA[i], 1, (d >> i) & 1 );

}


static void bf532_bf1_bus_printinfo (bus_t *bus) {
int i;

	for (i = 0; i < CHAIN->parts->len; i++)
		if (PART == CHAIN->parts->parts[i])
			break;
	printf( _("Blackfin BF532 compatible bus (with SPI bus) "
		"driver via BSR (JTAG part No. %d)\n"),
		i );
}

static void bf532_bf1_bus_prepare (bus_t *bus) {

	part_set_instruction (PART, "EXTEST");
	chain_shift_instructions (CHAIN);
	DPRINTF ("bus prepared\n");
}

static int bf532_bf1_bus_area (bus_t *bus, uint32_t adr, bus_area_t *area) {

	area-> description = NULL;
	area-> start = UINT32_C (0x00000000);
	area-> length = UINT64_C (0x100000000);
	area-> width = 16;

	return URJTAG_STATUS_OK;
}

static void
bf532_bf1_bus_read_start( bus_t *bus, uint32_t adr )
{
	part_t *p = PART;
	chain_t *chain = CHAIN;

	select_flash (bus);
	part_set_signal (p, AOE, 1, 0);
	part_set_signal (p, AWE, 1, 1);

	setup_address( bus, adr );
	set_data_in( bus );

	chain_shift_data_registers( chain, 0 );
}

static uint32_t
bf532_bf1_bus_read_next( bus_t *bus, uint32_t adr )
{
	part_t *p = PART;
	chain_t *chain = CHAIN;
	int i;
	uint32_t d = 0;

	setup_address( bus, adr );
	chain_shift_data_registers( chain, 1 );

	for (i = 0; i < 16; i++)
		d |= (uint32_t) (part_get_signal( p, DATA[i] ) << i);

	return d;
}

static uint32_t
bf532_bf1_bus_read_end( bus_t *bus )
{
	part_t *p = PART;
	chain_t *chain = CHAIN;
	int i;
	uint32_t d = 0;

	unselect_flash( bus );
	part_set_signal( p, AOE, 1, 1 );
	part_set_signal( p, AWE, 1, 1 );

	chain_shift_data_registers( chain, 1 );

	for (i = 0; i < 16; i++)
		d |= (uint32_t) (part_get_signal( p, DATA[i] ) << i);

	return d;
}

static uint32_t
bf532_bf1_bus_read( bus_t *bus, uint32_t adr )
{
	bf532_bf1_bus_read_start( bus, adr );
	return bf532_bf1_bus_read_end( bus );
}

static void
bf532_bf1_bus_write( bus_t *bus, uint32_t adr, uint32_t data )
{
	part_t *p = PART;
	chain_t *chain = CHAIN;

//	printf("Writing %04X to %08X...\n", data, adr);

	select_flash( bus );
	part_set_signal( p, AOE, 1, 1 );

	setup_address( bus, adr );
	setup_data( bus, data );

	chain_shift_data_registers( chain, 0 );

	part_set_signal( p, AWE, 1, 0 );
	chain_shift_data_registers( chain, 0 );
	part_set_signal( p, AWE, 1, 1 );
	unselect_flash( bus );
	chain_shift_data_registers( chain, 0 );
}

static void bf532_bf1_bus_free (bus_t *bus) {

	free (bus-> params);
	free (bus);
}

//static bus_t *bf532_bf1_bus_new (void);


//static bus_t *bf532_bf1_bus_new (void) {
static bus_t *bf532_bf1_bus_new ( chain_t *chain, const bus_driver_t *driver, char *cmd_params[] ) {
bus_t *bus;
part_t *part;
int failed = 0, i;
char buff [1024];

/*
	if (! chain) return NULL;
	if (! chain-> parts) return NULL;
	if (chain-> parts-> len <= chain-> active_part) return NULL;
	if (chain-> active_part < 0) return NULL;
*/
	bus = calloc (1, sizeof (bus_t));
	if (! bus) return NULL;

	//bus-> driver = & bf532_bf1_bus;
	bus-> driver = driver;
	bus-> params = calloc ( 1, sizeof (bus_params_t));
	if (! bus-> params) {
		free (bus);
		return NULL;
	}

	CHAIN = chain;
	PART = part = chain-> parts->parts [chain-> active_part];
	
	failed |= generic_bus_attach_sig( part, &(PF2),    "PF2"  );
	failed |= generic_bus_attach_sig( part, &(PF13),    "PF13"  );
	
	/*
	PF2 = part_find_signal (PART, "PF2");
	if (! PF2) {
		printf ( _("signal '%s' not found\n"), "PF2");
		failed = 1;
	}

	PF13 = part_find_signal (PART, "PF13");
	if (! PF13) {
		printf ( _("signal '%s' not found\n"), "PF13");
		failed = 1;
	}
	*/
	
	for (i = 0; i < 4; i++) {
		sprintf( buff, "AMS_B%d", i );
		failed |= generic_bus_attach_sig( part, &(AMS[i]), buff );
	}
	
	for (i = 0; i < 19; i++) {
		sprintf( buff, "ADDR[%d]", i + 1);
		failed |= generic_bus_attach_sig( part, &(ADDR[i]), buff );
	}

	for (i = 0; i < 16; i++) {
		sprintf( buff, "DATA[%d]", i);
		failed |= generic_bus_attach_sig( part, &(DATA[i]), buff );
	}

	failed |= generic_bus_attach_sig( part, &(AWE),    "AWE_B"  );

	failed |= generic_bus_attach_sig( part, &(AOE),    "AOE_B"  );

	//failed |= generic_bus_attach_sig( part, &(ABE[0]), "ABE_B0" );

	//failed |= generic_bus_attach_sig( part, &(ABE[1]), "ABE_B1" );

	failed |= generic_bus_attach_sig( part, &(SRAS),   "SRAS_B" );

	failed |= generic_bus_attach_sig( part, &(SCAS),   "SCAS_B" );

	failed |= generic_bus_attach_sig( part, &(SWE),    "SWE_B"  );

	failed |= generic_bus_attach_sig( part, &(SMS),    "SMS_B"  );
	
	/*
	for (i = 0; i < 4; i++) {
		sprintf( buff, "AMS_B%d", i );
		AMS[i] = part_find_signal( PART, buff );
		if (!AMS[i]) {
			printf( _("signal '%s' not found\n"), buff );
			failed = 1;
			break;
		}
	}
	
	for (i = 0; i < 19; i++) {
		sprintf( buff, "ADDR[%d]", i + 1);
		ADDR[i] = part_find_signal( PART, buff );
		if (!ADDR[i]) {
			printf( _("signal '%s' not found\n"), buff );
			failed = 1;
			break;
		}
	}
	for (i = 0; i < 16; i++) {
		sprintf( buff, "DATA[%d]", i);
		DATA[i] = part_find_signal( PART, buff );
		if (!DATA[i]) {
			printf( _("signal '%s' not found\n"), buff );
			failed = 1;
			break;
		}
	}

	AWE = part_find_signal( PART, "AWE_B" );
	if (!AWE) {
		printf( _("signal '%s' not found\n"), "AWE_B" );
		failed = 1;
	}

	AOE = part_find_signal( PART, "AOE_B" );
	if (!AOE) {
		printf( _("signal '%s' not found\n"), "AOE_B" );
		failed = 1;
	}

	SRAS = part_find_signal( PART, "SRAS_B" );
	if (!SRAS) {
		printf( _("signal '%s' not found\n"), "SRAS_B" );
		failed = 1;
	}

	SCAS = part_find_signal( PART, "SCAS_B" );
	if (!SCAS) {
		printf( _("signal '%s' not found\n"), "SCAS_B" );
		failed = 1;
	}

	SWE = part_find_signal( PART, "SWE_B" );
	if (!SWE) {
		printf( _("signal '%s' not found\n"), "SWE_B" );
		failed = 1;
	}

	SMS = part_find_signal( PART, "SMS_B" );
	if (!SMS) {
		printf( _("signal '%s' not found\n"), "SMS_B" );
		failed = 1;
	}
	*/
	
	failed |= generic_bus_attach_sig( part, &(MISO),    "MISO"  );
	failed |= generic_bus_attach_sig( part, &(MOSI),    "MOSI"  );
	failed |= generic_bus_attach_sig( part, &(SCK),    "SCK"  );
	
	/*
	MISO = part_find_signal (PART, "MISO");
	if (! MISO) {
		printf ( _("signal '%s' not found\n"), "MISO");
		failed = 1;
	}
	MOSI = part_find_signal (PART, "MOSI");
	if (! MOSI) {
		printf ( _("signal '%s' not found\n"), "MOSI");
		failed = 1;
	}
	SCK = part_find_signal (PART, "SCK");
	if (! SCK) {
		printf ( _("signal '%s' not found\n"), "SCK");
		failed = 1;
	}
	*/
	
	if (failed) {
		free (bus-> params);
		free (bus);
		return NULL;
	}

	return bus;
}

const bus_driver_t bf532_bf1_bus = {
	"bf532_bf1",
	N_("IP0X BF532 board SPI bus driver"),
	bf532_bf1_bus_new,
	bf532_bf1_bus_free,
	bf532_bf1_bus_printinfo,
	bf532_bf1_bus_prepare,
	bf532_bf1_bus_area,
	bf532_bf1_bus_read_start,
	bf532_bf1_bus_read_next,
	bf532_bf1_bus_read_end,
	bf532_bf1_bus_read,
	bf532_bf1_bus_write,
	generic_bus_no_init,
	bf532_bf1_bus_spi_send,
	bf532_bf1_bus_spi_recv,
	bf532_bf1_bus_spi_on,
	bf532_bf1_bus_spi_off
};

