/*
 * usb_desc.c - USB Descriptor
 */

#include <stdint.h>
#include <string.h>

#include "config.h"

#include "sys.h"
#include "usb_lld.h"
#include "usb_conf.h"
#include "usb-cdc.h"

#define W_LENGTH(x) ((x)&0xFF), (((x)>>8)&0xFF)

/* USB Standard Device Descriptor */
#if !defined(GNU_LINUX_EMULATION)
static const
#endif
uint8_t device_desc[] = {
  18,   /* bLength */
  DEVICE_DESCRIPTOR,     /* bDescriptorType */
  0x10, 0x01,            /* bcdUSB = 1.1 */
  0xef,   /* bDeviceClass: Misc device class */
  0x02,   /* bDeviceSubClass: Common class */
  0x01,   /* bDeviceProtocol: Interface association descriptor */
  0x40,   /* bMaxPacketSize0 */
#include "usb-vid-pid-ver.c.inc"
  1, /* Index of string descriptor describing manufacturer */
  2, /* Index of string descriptor describing product */
  3, /* Index of string descriptor describing the device's serial number */
  0x01    /* bNumConfigurations */
};

#ifdef ENABLE_VIRTUAL_COM_PORT
#define VCOM_TOTAL_LENGTH (8+9+5+5+4+5+7+9+7+7)
#else
#define VCOM_TOTAL_LENGTH   0
#endif

#define TOTAL_LENGTH (9+VCOM_TOTAL_LENGTH)


/* Configuation Descriptor */
static const uint8_t config_desc[] = {
  9,			   /* bLength: Configuation Descriptor size */
  CONFIG_DESCRIPTOR,	   /* bDescriptorType: Configuration */
  W_LENGTH(TOTAL_LENGTH),  /* wTotalLength:no of returned bytes */
  NUM_INTERFACES,	   /* bNumInterfaces: */
  0x01,   /* bConfigurationValue: Configuration value */
  0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
  USB_INITIAL_FEATURE,  /* bmAttributes*/
  50,	  /* MaxPower 100 mA */

#ifdef ENABLE_VIRTUAL_COM_PORT
  /* Interface Association Descriptor */
  8,			      /* bLength: Interface Association Descriptor size */
  0x0b,			      /* bDescriptorType: Interface Association */
  VCOM_INTERFACE_0,	      /* bInterfaceNumber of first interface */
  2,			      /* bInterfaceCount of contiguous interfaces */
  0x02,			      /* bFunctionClass: Communication Interface Class */
  0x02,			      /* bFunctionSubClass: Abstract Control Model */
  0x01,			      /* bInterfaceProtocol: Common AT commands */
  0x00,			      /* iFunction: */

  /* Interface Descriptor */
  9,			      /* bLength: Interface Descriptor size */
  INTERFACE_DESCRIPTOR,	      /* bDescriptorType: Interface */
  VCOM_INTERFACE_0,	 /* bInterfaceNumber: Index of Interface */
  0x00,		  /* bAlternateSetting: Alternate setting */
  0x01,		  /* bNumEndpoints: One endpoints used */
  0x02,		  /* bInterfaceClass: Communication Interface Class */
  0x02,		  /* bInterfaceSubClass: Abstract Control Model */
  0x01,		  /* bInterfaceProtocol: Common AT commands */
  0x00,		  /* iInterface: */
  /*Header Functional Descriptor*/
  5,			    /* bLength: Endpoint Descriptor size */
  0x24,			    /* bDescriptorType: CS_INTERFACE */
  0x00,			    /* bDescriptorSubtype: Header Func Desc */
  0x10, 0x01,		    /* bcdCDC: spec release number */
  /*Call Managment Functional Descriptor*/
  5,	    /* bFunctionLength */
  0x24,	    /* bDescriptorType: CS_INTERFACE */
  0x01,	    /* bDescriptorSubtype: Call Management Func Desc */
  0x03,	    /* bmCapabilities: D0+D1 */
  VCOM_INTERFACE_1,	    /* bDataInterface */
  /*ACM Functional Descriptor*/
  4,	    /* bFunctionLength */
  0x24,	    /* bDescriptorType: CS_INTERFACE */
  0x02,	    /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,	    /* bmCapabilities */
  /*Union Functional Descriptor*/
  5,		 /* bFunctionLength */
  0x24,		 /* bDescriptorType: CS_INTERFACE */
  0x06,		 /* bDescriptorSubtype: Union func desc */
  VCOM_INTERFACE_0,	 /* bMasterInterface: Communication class interface */
  VCOM_INTERFACE_1,	 /* bSlaveInterface0: Data Class Interface */
  /*Endpoint 4 Descriptor*/
  7,			       /* bLength: Endpoint Descriptor size */
  ENDPOINT_DESCRIPTOR,	       /* bDescriptorType: Endpoint */
  0x84,				   /* bEndpointAddress: (IN4) */
  0x03,				   /* bmAttributes: Interrupt */
  W_LENGTH(VIRTUAL_COM_PORT_INT_SIZE), /* wMaxPacketSize: */
  0xFF,				   /* bInterval: */

  /*Data class interface descriptor*/
  9,			       /* bLength: Endpoint Descriptor size */
  INTERFACE_DESCRIPTOR,	       /* bDescriptorType: */
  VCOM_INTERFACE_1,	 /* bInterfaceNumber: Index of Interface */
  0x00,			   /* bAlternateSetting: Alternate setting */
  0x02,			   /* bNumEndpoints: Two endpoints used */
  0x0A,			   /* bInterfaceClass: CDC */
  0x00,			   /* bInterfaceSubClass: */
  0x00,			   /* bInterfaceProtocol: */
  0x00,			   /* iInterface: */
  /*Endpoint 5 Descriptor*/
  7,			       /* bLength: Endpoint Descriptor size */
  ENDPOINT_DESCRIPTOR,	       /* bDescriptorType: Endpoint */
  0x05,				    /* bEndpointAddress: (OUT5) */
  0x02,				    /* bmAttributes: Bulk */
  W_LENGTH(VIRTUAL_COM_PORT_DATA_SIZE), /* wMaxPacketSize: */
  0x00,			     /* bInterval: ignore for Bulk transfer */
  /*Endpoint 3 Descriptor*/
  7,			       /* bLength: Endpoint Descriptor size */
  ENDPOINT_DESCRIPTOR,	       /* bDescriptorType: Endpoint */
  0x83,				    /* bEndpointAddress: (IN3) */
  0x02,				    /* bmAttributes: Bulk */
  W_LENGTH(VIRTUAL_COM_PORT_DATA_SIZE), /* wMaxPacketSize: */
  0x00,				    /* bInterval */
#endif
};


/* USB String Descriptors */
static const uint8_t template_string_lang_id[] = {
  4,				/* bLength */
  STRING_DESCRIPTOR,
  0x09, 0x04			/* LangID = 0x0409: US-English */
};

#define USB_STRINGS 1
#include "usb-strings.c.inc"

struct desc
{
  const uint8_t *desc;
  uint16_t size;
};

static const struct desc string_descriptors[] = {
  {template_string_lang_id, sizeof (template_string_lang_id)},
  {template_string_vendor, sizeof (template_string_vendor)},
  {template_string_product, sizeof (template_string_product)},
  {template_string_serial, sizeof (template_string_serial)},
  {template_revision_detail, sizeof (template_revision_detail)},
  {template_config_options, sizeof (template_config_options)},
  {sys_version, sizeof (sys_version)},
};
#define NUM_STRING_DESC (sizeof (string_descriptors) / sizeof (struct desc))

int
usb_get_descriptor (struct usb_dev *dev)
{
  struct device_req *arg = &dev->dev_req;
  uint8_t rcp = arg->type & RECIPIENT;
  uint8_t desc_type = (arg->value >> 8);
  uint8_t desc_index = (arg->value & 0xff);

  if (rcp == DEVICE_RECIPIENT)
    {
      if (desc_type == DEVICE_DESCRIPTOR)
	return usb_lld_ctrl_send (dev, device_desc, sizeof (device_desc));
      else if (desc_type == CONFIG_DESCRIPTOR)
	return usb_lld_ctrl_send (dev, config_desc, sizeof (config_desc));
      else if (desc_type == STRING_DESCRIPTOR)
	{
	  if (desc_index < NUM_STRING_DESC)
	    return usb_lld_ctrl_send (dev, string_descriptors[desc_index].desc,
				      string_descriptors[desc_index].size);
#ifdef USE_SYS3
	  else if (desc_index == NUM_STRING_DESC)
	    {
	      uint8_t usbbuf[64];
	      int i;
	      size_t len;

	      for (i = 0; i < (int)sizeof (usbbuf)/2 - 2; i++)
		{
		  if (sys_board_name[i] == 0)
		    break;

		  usbbuf[i*2+2] = sys_board_name[i];
		  usbbuf[i*2+3] = 0;
		}
	      usbbuf[0] = len = i*2 + 2;
	      usbbuf[1] = STRING_DESCRIPTOR;
	      return usb_lld_ctrl_send (dev, usbbuf, len);
	    }
#endif
	}
    }

  return -1;
}
