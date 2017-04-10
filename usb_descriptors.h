static
unsigned char hid_report_descriptor[] __attribute__ ((aligned(64))) = {
	0x05, 0x01,		// Usage Page (Generic Desktop Ctrls)
	0x09, 0x05,		// Usage (Game Pad)
	0xA1, 0x01,		// Collection (Application)
	0xA1, 0x00,		//   Collection (Physical)
	0x85, 0x01,		//     Report ID (1)
	0x05, 0x09,		//     Usage Page (Button)
	0x19, 0x01,		//     Usage Minimum (0x01)
	0x29, 0x10,		//     Usage Maximum (0x10)
	0x15, 0x00,		//     Logical Minimum (0)
	0x25, 0x01,		//     Logical Maximum (1)
	0x95, 0x10,		//     Report Count (16)
	0x75, 0x01,		//     Report Size (1)
	0x81, 0x02,		//     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x05, 0x01,		//     Usage Page (Generic Desktop Ctrls)
	0x09, 0x30,		//     Usage (X)
	0x09, 0x31,		//     Usage (Y)
	0x09, 0x32,		//     Usage (Z)
	0x09, 0x33,		//     Usage (Rx)
	0x15, 0x81,		//     Logical Minimum (-127)
	0x25, 0x7F,		//     Logical Maximum (127)
	0x75, 0x08,		//     Report Size (8)
	0x95, 0x04,		//     Report Count (4)
	0x81, 0x02,		//     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0xC0,			//   End Collection
	0xC0,			// End Collection
};

/* HID descriptor */
static
unsigned char hiddesc[] = {
	0x09,					/* bLength */
	HID_DESCRIPTOR_HID,			/* bDescriptorType */
	0x11, 0x01,				/* bcdHID */
	0x00,					/* bCountryCode */
	0x01,					/* bNumDescriptors */
	HID_DESCRIPTOR_REPORT,			/* bDescriptorType */
	sizeof(hid_report_descriptor), 0x00	/* wDescriptorLength */
};

/* Endpoint blocks */
static
struct SceUdcdEndpoint endpoints[2] = {
	{0x00, 0, 0, 0},
	{0x80, 1, 0, 0}
};

/* Interfaces */
static
struct SceUdcdInterface interfaces[1] = {
	{-1, 0, 1}
};

/* String descriptor */
static
struct SceUdcdStringDescriptor string_descriptors[2] = {
	{
		20,
		USB_DT_STRING,
		{'V', 'i', 't', 'a', 's', 't', 'i', 'c', 'k'}
	},
	{
		0,
		USB_DT_STRING
	}
};

/* HI-Speed device descriptor */
static
struct SceUdcdDeviceDescriptor devdesc_hi = {
	USB_DT_DEVICE_SIZE,
	USB_DT_DEVICE,
	0x200,			/* bcdUSB */
	USB_CLASS_PER_INTERFACE,	/* bDeviceClass */
	0,			/* bDeviceSubClass */
	0,			/* bDeviceProtocol */
	64,			/* bMaxPacketSize0 */
	0,			/* idProduct */
	0,			/* idVendor */
	0x100,			/* bcdDevice */
	0,			/* iManufacturer */
	0,			/* iProduct */
	0,			/* iSerialNumber */
	1			/* bNumConfigurations */
};

/* Hi-Speed endpoint descriptors */
static
struct SceUdcdEndpointDescriptor endpdesc_hi[2] = {
	{
		USB_DT_ENDPOINT_SIZE,
		USB_DT_ENDPOINT,
		0x81,			/* bEndpointAddress */
		0x03,			/* bmAttributes */
		0x40,			/* wMaxPacketSize */
		0x01			/* bInterval */
	},
	{
		0,
	}
};

/* Hi-Speed interface descriptor */
static
struct SceUdcdInterfaceDescriptor interdesc_hi[2] = {
	{
		USB_DT_INTERFACE_SIZE,
		USB_DT_INTERFACE,
		0,			/* bInterfaceNumber */
		0,			/* bAlternateSetting */
		1,			/* bNumEndpoints */
		USB_CLASS_HID,		/* bInterfaceClass */
		0x00,			/* bInterfaceSubClass */
		0x00,			/* bInterfaceProtocol */
		1,			/* iInterface */
		&endpdesc_hi[0],	/* endpoints */
		hiddesc,
		sizeof(hiddesc)
	},
	{
		0
	}
};

/* Hi-Speed settings */
static
struct SceUdcdInterfaceSettings settings_hi[1] = {
	{
		&interdesc_hi[0],
		0,
		1
	}
};

/* Hi-Speed configuration descriptor */
static
struct SceUdcdConfigDescriptor confdesc_hi = {
	USB_DT_CONFIG_SIZE,
	USB_DT_CONFIG,
	(USB_DT_INTERFACE_SIZE + USB_DT_CONFIG_SIZE + USB_DT_ENDPOINT_SIZE + sizeof(hiddesc)),	/* wTotalLength */
	1,			/* bNumInterfaces */
	1,			/* bConfigurationValue */
	0,			/* iConfiguration */
	0xC0,			/* bmAttributes */
	0,			/* bMaxPower */
	&settings_hi[0]
};

/* Hi-Speed configuration */
static
struct SceUdcdConfiguration config_hi = {
	&confdesc_hi,
	&settings_hi[0],
	&interdesc_hi[0],
	&endpdesc_hi[0]
};

/* Full-Speed device descriptor */
static
struct SceUdcdDeviceDescriptor devdesc_full = {
	USB_DT_DEVICE_SIZE,
	USB_DT_DEVICE,
	0x200,			/* bcdUSB (should be 0x110 but the PSVita freezes otherwise) */
	USB_CLASS_PER_INTERFACE,	/* bDeviceClass */
	0,			/* bDeviceSubClass */
	0,			/* bDeviceProtocol */
	8,			/* bMaxPacketSize0 */
	0,			/* idProduct */
	0,			/* idVendor */
	0x200,			/* bcdDevice */
	0,			/* iManufacturer */
	0,			/* iProduct */
	0,			/* iSerialNumber */
	1			/* bNumConfigurations */
};

/* Full-Speed endpoint descriptors */
static
struct SceUdcdEndpointDescriptor endpdesc_full[2] = {
	{
		USB_DT_ENDPOINT_SIZE,
		USB_DT_ENDPOINT,
		0x81,			/* bEndpointAddress */
		0x03,			/* bmAttributes */
		0x40,			/* wMaxPacketSize */
		0x01			/* bInterval */
	},
	{
		0
	}
};

/* Full-Speed interface descriptor */
static
struct SceUdcdInterfaceDescriptor interdesc_full[2] = {
	{
		USB_DT_INTERFACE_SIZE,
		USB_DT_INTERFACE,
		0,			/* bInterfaceNumber */
		0,			/* bAlternateSetting */
		1,			/* bNumEndpoints */
		USB_CLASS_HID,		/* bInterfaceClass */
		0x00,			/* bInterfaceSubClass */
		0x00,			/* bInterfaceProtocol */
		1,			/* iInterface */
		&endpdesc_full[0],	/* endpoints */
		hiddesc,
		sizeof(hiddesc)
	},
	{
		0
	}
};

/* Full-Speed settings */
static
struct SceUdcdInterfaceSettings settings_full[1] = {
	{
		&interdesc_full[0],
		0,
		1
	}
};

/* Full-Speed configuration descriptor */
static
struct SceUdcdConfigDescriptor confdesc_full = {
	USB_DT_CONFIG_SIZE,
	USB_DT_CONFIG,
	(USB_DT_INTERFACE_SIZE + USB_DT_CONFIG_SIZE + USB_DT_ENDPOINT_SIZE + sizeof(hiddesc)),	/* wTotalLength */
	1,			/* bNumInterfaces */
	1,			/* bConfigurationValue */
	0,			/* iConfiguration */
	0xC0,			/* bmAttributes */
	0,			/* bMaxPower */
	&settings_full[0]
};

/* Full-Speed configuration */
static
struct SceUdcdConfiguration config_full = {
	&confdesc_full,
	&settings_full[0],
	&interdesc_full[0],
	&endpdesc_full[0]
};
