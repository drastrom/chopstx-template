/*
 * usb-thread.c -- USB protocol handling
 *
 * Copyright (C) 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
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

#ifdef DEBUG
#include "usb-cdc.h"
#include "debug.h"
struct stdout stdout;
#endif

#include "usb_lld.h"
#include "usb_conf.h"

/* shit from gnuk.h */
#define LED_FINISH_COMMAND	128
#define LED_OFF	 LED_FINISH_COMMAND
void led_blink(int spec);



#if defined(DEBUG) && defined(GNU_LINUX_EMULATION)
static uint8_t endp5_buf[VIRTUAL_COM_PORT_DATA_SIZE];
#endif

static void
usb_rx_ready (uint8_t ep_num, uint16_t len)
{
#ifdef DEBUG
  if (ep_num == ENDP5)
    {
      chopstx_mutex_lock (&stdout.m_dev);
#ifdef GNU_LINUX_EMULATION
      usb_lld_rx_enable_buf (ep_num, endp5_buf, VIRTUAL_COM_PORT_DATA_SIZE);
#else
      usb_lld_rx_enable (ep_num);
#endif
      chopstx_mutex_unlock (&stdout.m_dev);
    }
#endif
}

static void
usb_tx_done (uint8_t ep_num, uint16_t len)
{
#ifdef DEBUG
  if (ep_num == ENDP3)
    {
      chopstx_mutex_lock (&stdout.m_dev);
      chopstx_cond_signal (&stdout.cond_dev);
      chopstx_mutex_unlock (&stdout.m_dev);
    }
#endif
}

#define USB_TIMEOUT (1950*1000)

extern uint32_t bDeviceState;
extern void usb_device_reset (struct usb_dev *dev);
extern int usb_setup (struct usb_dev *dev);
extern void usb_ctrl_write_finish (struct usb_dev *dev);
extern int usb_set_configuration (struct usb_dev *dev);
extern int usb_set_interface (struct usb_dev *dev);
extern int usb_get_interface (struct usb_dev *dev);
extern int usb_get_status_interface (struct usb_dev *dev);

extern int usb_get_descriptor (struct usb_dev *dev);

static chopstx_intr_t interrupt;

/*
 * Return 0 for normal USB event
 *       -1 for USB reset
 *        1 for SET_INTERFACE or SET_CONFIGURATION
 */
static int
usb_event_handle (struct usb_dev *dev)
{
  uint8_t ep_num;
  int e;

  e = usb_lld_event_handler (dev);
  chopstx_intr_done(&interrupt);
  ep_num = USB_EVENT_ENDP (e);

  /* Transfer to endpoint (not control endpoint) */
  if (ep_num != 0)
    {
      if (USB_EVENT_TXRX (e))
	usb_tx_done (ep_num, USB_EVENT_LEN (e));
      else
	usb_rx_ready (ep_num, USB_EVENT_LEN (e));
      return 0;
    }

  /* Control endpoint */
  switch (USB_EVENT_ID (e))
    {
    case USB_EVENT_DEVICE_RESET:
      usb_device_reset (dev);
      return -1;

    case USB_EVENT_DEVICE_ADDRESSED:
      bDeviceState = USB_DEVICE_STATE_ADDRESSED;
      break;

    case USB_EVENT_GET_DESCRIPTOR:
      if (usb_get_descriptor (dev) < 0)
	usb_lld_ctrl_error (dev);
      break;

    case USB_EVENT_SET_CONFIGURATION:
      if (usb_set_configuration (dev) < 0)
	usb_lld_ctrl_error (dev);
      else
	{
	  if (bDeviceState == USB_DEVICE_STATE_ADDRESSED)
	    /* de-Configured */
	    return -1;
	  else
	    /* Configured */
	    return 1;
	}
      break;

    case USB_EVENT_SET_INTERFACE:
      if (usb_set_interface (dev) < 0)
	usb_lld_ctrl_error (dev);
      else
	return 1;
      break;

    case USB_EVENT_CTRL_REQUEST:
      /* Device specific device request.  */
      if (usb_setup (dev) < 0)
	usb_lld_ctrl_error (dev);
      break;

    case USB_EVENT_GET_STATUS_INTERFACE:
      if (usb_get_status_interface (dev) < 0)
	usb_lld_ctrl_error (dev);
      break;

    case USB_EVENT_GET_INTERFACE:
      if (usb_get_interface (dev) < 0)
	usb_lld_ctrl_error (dev);
      break;

    case USB_EVENT_SET_FEATURE_DEVICE:
    case USB_EVENT_SET_FEATURE_ENDPOINT:
    case USB_EVENT_CLEAR_FEATURE_DEVICE:
    case USB_EVENT_CLEAR_FEATURE_ENDPOINT:
      usb_lld_ctrl_ack (dev);
      break;

    case USB_EVENT_CTRL_WRITE_FINISH:
      /* Control WRITE transfer finished.  */
      usb_ctrl_write_finish (dev);
      break;

    case USB_EVENT_DEVICE_SUSPEND:
      led_blink (LED_OFF);
      chopstx_usec_wait (10);	/* Make sure LED off */
      chopstx_conf_idle (2);
      bDeviceState |= USB_DEVICE_STATE_SUSPEND;
      break;

    case USB_EVENT_DEVICE_WAKEUP:
      chopstx_conf_idle (1);
      bDeviceState &= ~USB_DEVICE_STATE_SUSPEND;
      break;

    case USB_EVENT_OK:
    default:
      break;
    }

  return 0;
}

static struct chx_poll_head *const usb_poll[] = {
  (struct chx_poll_head *const)&interrupt
};
#define USB_POLL_NUM (sizeof (usb_poll)/sizeof (struct chx_poll_head *))

void *
usb_thread (void *arg)
{
  uint32_t timeout;
  struct usb_dev dev;
  uint32_t *timeout_p;

  (void)arg;

  usb_lld_init (&dev, USB_INITIAL_FEATURE);
  chopstx_claim_irq (&interrupt, INTR_REQ_USB);
  usb_event_handle (&dev);	/* For old SYS < 3.0 */

 reset:
  timeout = USB_TIMEOUT;
  while (1)
    {
      if (bDeviceState == USB_DEVICE_STATE_CONFIGURED)
	timeout_p = &timeout;
      else
	timeout_p = NULL;

      chopstx_poll (timeout_p, USB_POLL_NUM, usb_poll);

      if (interrupt.ready)
	{
	  if (usb_event_handle (&dev) == 0)
	    continue;

	  /* RESET handling:
	   * (1) After DEVICE_RESET, it needs to re-start out of the loop.
	   * (2) After SET_CONFIGURATION or SET_INTERFACE, the
	   *     endpoint is reset to RX_NAK.  It needs to prepare
	   *     receive again.
	   */
	  goto reset;
	}

      timeout = USB_TIMEOUT;
    }

  /* Loading reGNUal.  */
  while (bDeviceState != USB_DEVICE_STATE_UNCONNECTED)
    {
      chopstx_intr_wait (&interrupt);
      usb_event_handle (&dev);
    }

  return NULL;
}


#ifdef DEBUG
#include "usb-cdc.h"

void
stdout_init (void)
{
  chopstx_mutex_init (&stdout.m);
  chopstx_mutex_init (&stdout.m_dev);
  chopstx_cond_init (&stdout.cond_dev);
  stdout.connected = 0;
}

void
_write (const char *s, int len)
{
  int packet_len;

  if (len == 0)
    return;

  chopstx_mutex_lock (&stdout.m);

  chopstx_mutex_lock (&stdout.m_dev);
  if (!stdout.connected)
    chopstx_cond_wait (&stdout.cond_dev, &stdout.m_dev);
  chopstx_mutex_unlock (&stdout.m_dev);

  do
    {
      packet_len =
	(len < VIRTUAL_COM_PORT_DATA_SIZE) ? len : VIRTUAL_COM_PORT_DATA_SIZE;

      chopstx_mutex_lock (&stdout.m_dev);
#ifdef GNU_LINUX_EMULATION
      usb_lld_tx_enable_buf (ENDP3, s, packet_len);
#else
      usb_lld_write (ENDP3, s, packet_len);
#endif
      chopstx_cond_wait (&stdout.cond_dev, &stdout.m_dev);
      chopstx_mutex_unlock (&stdout.m_dev);

      s += packet_len;
      len -= packet_len;
    }
  /* Send a Zero-Length-Packet if the last packet is full size.  */
  while (len != 0 || packet_len == VIRTUAL_COM_PORT_DATA_SIZE);

  chopstx_mutex_unlock (&stdout.m);
}

#else
void
_write (const char *s, int size)
{
  (void)s;
  (void)size;
}
#endif
