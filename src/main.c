/*
 * main.c - main routine of Gnuk
 *
 * Copyright (C) 2010, 2011, 2012, 2013, 2015, 2016, 2017
 *               Free Software Initiative of Japan
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * This file is a part of Gnuk, a GnuPG USB Token implementation.
 *
 * Gnuk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Gnuk is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <string.h>
#include <chopstx.h>
#include <eventflag.h>

#include "config.h"

#include "sys.h"
#include "usb_lld.h"
#include "usb-cdc.h"
#ifdef GNU_LINUX_EMULATION
#include <stdio.h>
#include <stdlib.h>
#define main emulated_main
#else
#include "mcu/stm32f103.h"
#endif

/* shit from gnuk.h */
extern const uint8_t template_string_serial[];

#define LED_ONESHOT		  1
#define LED_TWOSHOTS		  2
#define LED_SHOW_STATUS		  4
#define LED_FATAL		  8
#define LED_SYNC	         16
#define LED_GNUK_EXEC		 32
#define LED_START_COMMAND	 64
#define LED_FINISH_COMMAND	128
#define LED_OFF	 LED_FINISH_COMMAND

extern uint8_t _regnual_start, __heap_end__[];


/*
 * main thread does 1-bit LED display output
 */
#define LED_TIMEOUT_INTERVAL	(75*1000)
#define LED_TIMEOUT_ZERO	(25*1000)
#define LED_TIMEOUT_ONE		(100*1000)
#define LED_TIMEOUT_STOP	(200*1000)


#ifdef GNU_LINUX_EMULATION
uint8_t *flash_addr_key_storage_start;
uint8_t *flash_addr_data_storage_start;
#else
#define ID_OFFSET (2+SERIALNO_STR_LEN*2)
static void
device_initialize_once (void)
{
  const uint8_t *p = &template_string_serial[ID_OFFSET];

  if (p[0] == 0xff && p[1] == 0xff && p[2] == 0xff && p[3] == 0xff)
    {
      /*
       * This is the first time invocation.
       * Setup serial number by unique device ID.
       */
      const uint8_t *u = unique_device_id () + 8;
      int i;

      for (i = 0; i < 4; i++)
	{
	  uint8_t b = u[3-i];
	  uint8_t nibble;

	  nibble = (b >> 4);
	  nibble += (nibble >= 10 ? ('A' - 10) : '0');
	  flash_program_halfword ((uintptr_t)&p[i*4], nibble);
	  nibble = (b & 0x0f);
	  nibble += (nibble >= 10 ? ('A' - 10) : '0');
	  flash_program_halfword ((uintptr_t)&p[i*4+2], nibble);
	}
    }
}
#endif


static volatile uint8_t fatal_code;
static struct eventflag led_event;
static chopstx_poll_cond_t led_event_poll_desc;
static struct chx_poll_head *const led_event_poll[] = {
  (struct chx_poll_head *)&led_event_poll_desc
};

static void display_fatal_code (void)
{
  while (1)
    {
      set_led (1);
      chopstx_usec_wait (LED_TIMEOUT_ZERO);
      set_led (0);
      chopstx_usec_wait (LED_TIMEOUT_INTERVAL);
      set_led (1);
      chopstx_usec_wait (LED_TIMEOUT_ZERO);
      set_led (0);
      chopstx_usec_wait (LED_TIMEOUT_INTERVAL);
      set_led (1);
      chopstx_usec_wait (LED_TIMEOUT_ZERO);
      set_led (0);
      chopstx_usec_wait (LED_TIMEOUT_STOP);
      set_led (1);
      if (fatal_code & 1)
	chopstx_usec_wait (LED_TIMEOUT_ONE);
      else
	chopstx_usec_wait (LED_TIMEOUT_ZERO);
      set_led (0);
      chopstx_usec_wait (LED_TIMEOUT_INTERVAL);
      set_led (1);
      if (fatal_code & 2)
	chopstx_usec_wait (LED_TIMEOUT_ONE);
      else
	chopstx_usec_wait (LED_TIMEOUT_ZERO);
      set_led (0);
      chopstx_usec_wait (LED_TIMEOUT_INTERVAL);
      set_led (1);
      chopstx_usec_wait (LED_TIMEOUT_STOP);
      set_led (0);
      chopstx_usec_wait (LED_TIMEOUT_INTERVAL*10);
    }
}

static uint8_t led_inverted;

static void
emit_led (uint32_t on_time, uint32_t off_time)
{
  set_led (!led_inverted);
  chopstx_poll (&on_time, 1, led_event_poll);
  set_led (led_inverted);
  chopstx_poll (&off_time, 1, led_event_poll);
}

void
led_blink (int spec)
{
  if (spec == LED_START_COMMAND || spec == LED_FINISH_COMMAND)
    {
      led_inverted = (spec == LED_START_COMMAND);
      spec = LED_SYNC;
    }

  eventflag_signal (&led_event, spec);
}

#ifdef FLASH_UPGRADE_SUPPORT
/*
 * In Gnuk 1.0.[12], reGNUal was not relocatable.
 * Now, it's relocatable, but we need to calculate its entry address
 * based on it's pre-defined address.
 */
#define REGNUAL_START_ADDRESS_COMPATIBLE 0x20001400
static uintptr_t
calculate_regnual_entry_address (const uint8_t *addr)
{
  const uint8_t *p = addr + 4;
  uintptr_t v = p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);

  v -= REGNUAL_START_ADDRESS_COMPATIBLE;
  v += (uintptr_t)addr;
  return v;
}
#endif

#define STACK_MAIN
#define STACK_PROCESS_1
#include "stack-def.h"
#define STACK_ADDR_USB ((uintptr_t)process1_base)
#define STACK_SIZE_USB (sizeof process1_base)

#define PRIO_USB 3
#define PRIO_MAIN 5

extern void *usb_thread (void *arg);

extern uint32_t bDeviceState;

/*
 * Entry point.
 */
int
main (int argc, const char *argv[])
{
#ifdef GNU_LINUX_EMULATION
  uintptr_t flash_addr;
  const char *flash_image_path;
  char *path_string = NULL;
#endif
#ifdef FLASH_UPGRADE_SUPPORT
  uintptr_t entry;
#endif
  chopstx_t usb_thd;

  chopstx_conf_idle (1);

#ifdef GNU_LINUX_EMULATION
#define FLASH_IMAGE_NAME ".flash-image"

  if (argc >= 4 || (argc == 2 && !strcmp (argv[1], "--help")))
    {
      fprintf (stdout, "Usage: %s [--vidpid=Vxxx:Pxxx] [flash-image-file]",
	       argv[0]);
      exit (0);
    }

  if (argc >= 2 && !strncmp (argv[1], "--debug=", 8))
    {
      debug = strtol (&argv[1][8], NULL, 10);
      argc--;
      argv++;
    }

  if (argc >= 2 && !strncmp (argv[1], "--vidpid=", 9))
    {
      extern uint8_t device_desc[];
      uint32_t id;
      char *p;

      id = (uint32_t)strtol (&argv[1][9], &p, 16);
      device_desc[8] = (id & 0xff);
      device_desc[9] = (id >> 8);

      if (p && p[0] == ':')
	{
	  id = (uint32_t)strtol (&p[1], NULL, 16);
	  device_desc[10] = (id & 0xff);
	  device_desc[11] = (id >> 8);
	}

      argc--;
      argv++;
    }

  if (argc == 1)
    {
      char *p = getenv ("HOME");

      if (p == NULL)
	{
	  fprintf (stderr, "Can't find $HOME\n");
	  exit (1);
	}

      path_string = malloc (strlen (p) + strlen (FLASH_IMAGE_NAME) + 2);

      p = stpcpy (path_string, p);
      *p++ = '/';
      strcpy (p, FLASH_IMAGE_NAME);
      flash_image_path = path_string;
    }
  else
    flash_image_path = argv[1];

  flash_addr = flash_init (flash_image_path);
  flash_addr_key_storage_start = (uint8_t *)flash_addr;
  flash_addr_data_storage_start = (uint8_t *)flash_addr + 4096;
#else
  (void)argc;
  (void)argv;
#endif

  flash_unlock ();

#ifdef GNU_LINUX_EMULATION
    if (path_string)
      free (path_string);
#else
  device_initialize_once ();
#endif

  eventflag_init (&led_event);

#ifdef DEBUG
  stdout_init ();
#endif

  usb_thd = chopstx_create (PRIO_USB, STACK_ADDR_USB, STACK_SIZE_USB,
			     usb_thread, NULL);

  chopstx_setpriority (PRIO_MAIN);

  while (1)
    {
      if (bDeviceState != USB_DEVICE_STATE_UNCONNECTED)
	break;

      chopstx_usec_wait (250*1000);
    }

  eventflag_prepare_poll (&led_event, &led_event_poll_desc);

  while (1)
    {
      eventmask_t m;

      m = eventflag_wait (&led_event);
      switch (m)
	{
	case LED_ONESHOT:
	  emit_led (100*1000, LED_TIMEOUT_STOP);
	  break;
	case LED_TWOSHOTS:
	  emit_led (50*1000, 50*1000);
	  emit_led (50*1000, LED_TIMEOUT_STOP);
	  break;
	case LED_SHOW_STATUS:
	  /*display_status_code ();*/
	  break;
	case LED_FATAL:
	  display_fatal_code ();
	  break;
	case LED_SYNC:
	  set_led (led_inverted);
	  break;
	case LED_GNUK_EXEC:
	  goto exec;
	default:
	  emit_led (LED_TIMEOUT_ZERO, LED_TIMEOUT_STOP);
	  break;
	}
    }

 exec:

  set_led (1);
  usb_lld_shutdown ();

  /* Finish application.  */
  chopstx_join (usb_thd, NULL);

#ifdef FLASH_UPGRADE_SUPPORT
  /* Set vector */
  SCB->VTOR = (uintptr_t)&_regnual_start;
  entry = calculate_regnual_entry_address (&_regnual_start);
#ifdef DFU_SUPPORT
#define FLASH_SYS_START_ADDR 0x08000000
#define FLASH_SYS_END_ADDR (0x08000000+0x1000)
#define CHIP_ID_REG ((uint32_t *)0xE0042000)
  {
    extern uint8_t _sys;
    uintptr_t addr;
    handler *new_vector = (handler *)FLASH_SYS_START_ADDR;
    void (*func) (void (*)(void)) = (void (*)(void (*)(void)))new_vector[9];
    uint32_t flash_page_size = 1024; /* 1KiB default */

   if ((*CHIP_ID_REG)&0x07 == 0x04) /* High dencity device.  */
     flash_page_size = 2048; /* It's 2KiB. */

    /* Kill DFU */
    for (addr = FLASH_SYS_START_ADDR; addr < FLASH_SYS_END_ADDR;
	 addr += flash_page_size)
      flash_erase_page (addr);

    /* copy system service routines */
    flash_write (FLASH_SYS_START_ADDR, &_sys, 0x1000);

    /* Leave Gnuk to exec reGNUal */
    (*func) ((void (*)(void))entry);
    for (;;);
  }
#else
  /* Leave Gnuk to exec reGNUal */
  flash_erase_all_and_exec ((void (*)(void))entry);
#endif
#else
  exit (0);
#endif

  /* Never reached */
  return 0;
}

void
fatal (uint8_t code)
{
  extern void _write (const char *s, int len);

  fatal_code = code;
  eventflag_signal (&led_event, LED_FATAL);
  _write ("fatal\r\n", 7);
  for (;;);
}
