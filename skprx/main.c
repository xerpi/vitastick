#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/udcd.h>
#include <psp2kern/ctrl.h>
#include <psp2/touch.h>
#include <psp2/motion.h>
#include <taihen.h>
#include "uapi/vitastick_uapi.h"
#include "usb_descriptors.h"
#include "log.h"

#define VITASTICK_DRIVER_NAME	"VITASTICK"
#define VITASTICK_USB_PID	0x1337

struct gamepad_report_t {
	uint8_t report_id;
	uint16_t buttons;
	int8_t left_x;
	int8_t left_y;
	int8_t right_x;
	int8_t right_y;
} __attribute__((packed));

#define EVF_CONNECTED		(1 << 0)
#define EVF_DISCONNECTED	(1 << 1)
#define EVF_EXIT		(1 << 2)
#define EVF_INT_REQ_COMPLETED	(1 << 3)
#define EVF_ALL_MASK		(EVF_INT_REQ_COMPLETED | (EVF_INT_REQ_COMPLETED - 1))

static SceUID usb_thread_id;
static SceUID usb_event_flag_id;
static int vitastick_driver_registered = 0;
static int vitastick_driver_activated = 0;

static int send_hid_report_desc(void)
{
	static SceUdcdDeviceRequest req = {
		.endpoint = &endpoints[0],
		.data = hid_report_descriptor,
		.unk = 0,
		.size = sizeof(hid_report_descriptor),
		.isControlRequest = 0,
		.onComplete = NULL,
		.transmitted = 0,
		.returnCode = 0,
		.next = NULL,
		.unused = NULL,
		.physicalAddress = NULL
	};

	return ksceUdcdReqSend(&req);
}

static int send_string_descriptor(int index)
{
	static SceUdcdDeviceRequest req = {
		.endpoint = &endpoints[0],
		.data = &string_descriptors[0],
		.unk = 0,
		.size = sizeof(string_descriptors[0]),
		.isControlRequest = 0,
		.onComplete = NULL,
		.transmitted = 0,
		.returnCode = 0,
		.next = NULL,
		.unused = NULL,
		.physicalAddress = NULL
	};

	return ksceUdcdReqSend(&req);
}

static int send_hid_report_init(uint8_t report_id)
{
	static struct gamepad_report_t gamepad __attribute__((aligned(64))) = {
		.report_id = 1,
		.buttons = 0,
		.left_x = 0,
		.left_y = 0,
		.right_x = 0,
		.right_y = 0
	};

	static SceUdcdDeviceRequest req = {
		.endpoint = &endpoints[0],
		.data = &gamepad,
		.unk = 0,
		.size = sizeof(gamepad),
		.isControlRequest = 0,
		.onComplete = NULL,
		.transmitted = 0,
		.returnCode = 0,
		.next = NULL,
		.unused = NULL,
		.physicalAddress = NULL
	};

	return ksceUdcdReqSend(&req);
}

static void hid_report_on_complete(SceUdcdDeviceRequest *req)
{
	LOG("hid_report_on_complete\n");

	ksceKernelSetEventFlag(usb_event_flag_id, EVF_INT_REQ_COMPLETED);
}

static void fill_gamepad_report(const SceCtrlData *pad, struct gamepad_report_t *gamepad)
{
	gamepad->report_id = 1;
	gamepad->buttons = 0;

	if (pad->buttons & SCE_CTRL_CROSS)
		gamepad->buttons |= 1 << 0;
	if (pad->buttons & SCE_CTRL_CIRCLE)
		gamepad->buttons |= 1 << 1;
	if (pad->buttons & SCE_CTRL_SQUARE)
		gamepad->buttons |= 1 << 2;
	if (pad->buttons & SCE_CTRL_TRIANGLE)
		gamepad->buttons |= 1 << 3;

	if (pad->buttons & SCE_CTRL_LTRIGGER)
		gamepad->buttons |= 1 << 4;
	if (pad->buttons & SCE_CTRL_RTRIGGER)
		gamepad->buttons |= 1 << 5;

	if (pad->buttons & SCE_CTRL_SELECT)
		gamepad->buttons |= 1 << 6;
	if (pad->buttons & SCE_CTRL_START)
		gamepad->buttons |= 1 << 7;

	if (pad->buttons & SCE_CTRL_UP)
		gamepad->buttons |= 1 << 8;
	if (pad->buttons & SCE_CTRL_DOWN)
		gamepad->buttons |= 1 << 9;
	if (pad->buttons & SCE_CTRL_RIGHT)
		gamepad->buttons |= 1 << 10;
	if (pad->buttons & SCE_CTRL_LEFT)
		gamepad->buttons |= 1 << 11;

	gamepad->left_x = (int8_t)pad->lx - 128;
	gamepad->left_y = (int8_t)pad->ly - 128;
	gamepad->right_x = (int8_t)pad->rx - 128;
	gamepad->right_y = (int8_t)pad->ry - 128;
}

static int send_hid_report(uint8_t report_id)
{
	static struct gamepad_report_t gamepad __attribute__((aligned(64))) = { 0 } ;
	SceCtrlData pad;

	ksceCtrlPeekBufferPositive(0, &pad, 1);

	fill_gamepad_report(&pad, &gamepad);

	ksceKernelCpuDcacheAndL2WritebackRange(&gamepad, sizeof(gamepad));

	static SceUdcdDeviceRequest req = {
		.endpoint = &endpoints[1],
		.data = &gamepad,
		.unk = 0,
		.size = sizeof(gamepad),
		.isControlRequest = 0,
		.onComplete = hid_report_on_complete,
		.transmitted = 0,
		.returnCode = 0,
		.next = NULL,
		.unused = NULL,
		.physicalAddress = NULL
	};

	return TEST_CALL(ksceUdcdReqSend, &req);
}

static int vitastick_udcd_process_request(int recipient, int arg, SceUdcdEP0DeviceRequest *req)
{
	LOG("usb_driver_process_request\n");

	LOG("recvctl: %x %x\n", recipient, arg);
	LOG("request: %x type: %x wValue: %x wIndex: %x wLength: %x\n",
		req->bRequest, req->bmRequestType, req->wValue, req->wIndex, req->wLength);

	if (arg < 0)
		return -1;

	uint8_t req_dir = req->bmRequestType & USB_CTRLTYPE_DIR_MASK;
	uint8_t req_type = req->bmRequestType & USB_CTRLTYPE_TYPE_MASK;
	uint8_t req_recipient = req->bmRequestType & USB_CTRLTYPE_REC_MASK;

	if (req_dir == USB_CTRLTYPE_DIR_DEVICE2HOST) {
		LOG("Device to host\n");

		switch (req_type) {
		case USB_CTRLTYPE_TYPE_STANDARD:
			switch (req_recipient) {
			case USB_CTRLTYPE_REC_DEVICE:
				switch (req->bRequest) {
				case USB_REQ_GET_DESCRIPTOR: {
					uint8_t descriptor_type = (req->wValue >> 8) & 0xFF;
					uint8_t descriptor_idx = req->wValue & 0xFF;

					switch (descriptor_type) {
					case USB_DT_STRING:
						send_string_descriptor(descriptor_idx);
						break;
					}
					break;
				}

				}
				break;
			case USB_CTRLTYPE_REC_INTERFACE:
				switch (req->bRequest) {
				case USB_REQ_GET_DESCRIPTOR: {
					uint8_t descriptor_type = (req->wValue >> 8) & 0xFF;
					uint8_t descriptor_idx = req->wValue & 0xFF;

					switch (descriptor_type) {
					case HID_DESCRIPTOR_REPORT:
						send_hid_report_desc();
						break;
					}
				}

				}
				break;
			}
			break;
		case USB_CTRLTYPE_TYPE_CLASS:
			switch (recipient) {
			case USB_CTRLTYPE_REC_INTERFACE:
				switch (req->bRequest) {
				case HID_REQUEST_GET_REPORT: {
					uint8_t report_type = (req->wValue >> 8) & 0xFF;
					uint8_t report_id = req->wValue & 0xFF;

					if (report_type == 1)/* Input report type */
						send_hid_report_init(report_id);
					break;
				}

				}
				break;
			}
			break;
		}
	} else if (req_dir == USB_CTRLTYPE_DIR_HOST2DEVICE) {
		LOG("Host to device\n");

		switch (req_type) {
		case USB_CTRLTYPE_TYPE_CLASS:
			switch (req_recipient) {
			case USB_CTRLTYPE_REC_INTERFACE:
				switch (req->bRequest) {
				case HID_REQUEST_SET_IDLE:
					LOG("Set idle!\n");
					break;
				}
				break;
			}
			break;
		}
	}

	return 0;
}

static int vitastick_udcd_change_setting(int interfaceNumber, int alternateSetting)
{
	LOG("vitastick_udcd_change %d %d\n", interfaceNumber, alternateSetting);

	return 0;
}

static int vitastick_udcd_attach(int usb_version)
{
	LOG("vitastick_udcd_attach %d\n", usb_version);

	ksceUdcdReqCancelAll(&endpoints[1]);
	ksceUdcdClearFIFO(&endpoints[1]);

	ksceKernelSetEventFlag(usb_event_flag_id, EVF_CONNECTED);

	return 0;
}

static void vitastick_udcd_detach(void)
{
	LOG("vitastick_udcd_detach\n");

	ksceKernelSetEventFlag(usb_event_flag_id, EVF_DISCONNECTED);
}

static void vitastick_udcd_configure(int usb_version, int desc_count, SceUdcdInterfaceSettings *settings)
{
	LOG("vitastick_udcd_configure %d %d %p %d\n", usb_version, desc_count, settings, settings->numDescriptors);
}

static int vitastick_driver_start(int size, void *p)
{
	LOG("vitastick_driver_start\n");

	return 0;
}

static int vitastick_driver_stop(int size, void *p)
{
	LOG("vitastick_driver_stop\n");

	return 0;
}

static SceUdcdDriver vitastick_udcd_driver = {
	VITASTICK_DRIVER_NAME,
	2,
	&endpoints[0],
	&interfaces[0],
	&devdesc_hi,
	&config_hi,
	&devdesc_full,
	&config_full,
	&string_descriptors[0],
	&string_descriptors[0],
	&string_descriptors[0],
	&vitastick_udcd_process_request,
	&vitastick_udcd_change_setting,
	&vitastick_udcd_attach,
	&vitastick_udcd_detach,
	&vitastick_udcd_configure,
	&vitastick_driver_start,
	&vitastick_driver_stop,
	0,
	0,
	NULL
};

static int usb_thread(SceSize args, void *argp)
{
	int connected = 0;

	ksceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

	while (1) {
		int ret;
		unsigned int evf_out;

		ret = ksceKernelWaitEventFlagCB(usb_event_flag_id, EVF_ALL_MASK,
			SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR_PAT, &evf_out, NULL);
		if (ret < 0)
			break;

		LOG("Event flag: 0x%08X\n", evf_out);

		if (evf_out & EVF_EXIT) {
			break;
		} else if (evf_out & EVF_CONNECTED) {
			if (send_hid_report(1) >= 0)
				connected = 1;
		} else if (evf_out & EVF_DISCONNECTED) {
			connected = 0;
		} else if (evf_out & EVF_INT_REQ_COMPLETED && connected) {
			send_hid_report(1);
		}
	}

	return 0;
}

/*
 * Exports to userspace
 */
int vitastick_start(void)
{
	unsigned long state;
	int ret;

	ENTER_SYSCALL(state);

	log_reset();

	LOG("vitastick_start\n");

	if (!vitastick_driver_registered) {
		ret =  VITASTICK_ERROR_DRIVER_NOT_REGISTERED;
		goto err;
	} else if (vitastick_driver_activated) {
		ret = VITASTICK_ERROR_DRIVER_ALREADY_ACTIVATED;
		goto err;
	}

	ret = ksceUdcdDeactivate();
	if (ret < 0 && ret != SCE_UDCD_ERROR_INVALID_ARGUMENT) {
		LOG("Error deactivating UDCD (0x%08X)\n", ret);
		goto err;
	}

	ksceUdcdStop("USB_MTP_Driver", 0, NULL);
	ksceUdcdStop("USBPSPCommunicationDriver", 0, NULL);
	ksceUdcdStop("USBSerDriver", 0, NULL);
	ksceUdcdStop("USBDeviceControllerDriver", 0, NULL);

	ret = ksceUdcdStart("USBDeviceControllerDriver", 0, NULL);
	if (ret < 0) {
		LOG("Error starting the USBDeviceControllerDriver driver (0x%08X)\n", ret);
		goto err;
	}

	ret = ksceUdcdStart(VITASTICK_DRIVER_NAME, 0, NULL);
	if (ret < 0) {
		LOG("Error starting the " VITASTICK_DRIVER_NAME " driver (0x%08X)\n", ret);
		ksceUdcdStop("USBDeviceControllerDriver", 0, NULL);
		goto err;
	}

	ksceKernelClearEventFlag(usb_event_flag_id, ~EVF_ALL_MASK);

	ret = ksceUdcdActivate(VITASTICK_USB_PID);
	if (ret < 0) {
		LOG("Error activating the " VITASTICK_DRIVER_NAME " driver (0x%08X)\n", ret);
		ksceUdcdStop(VITASTICK_DRIVER_NAME, 0, NULL);
		ksceUdcdStop("USBDeviceControllerDriver", 0, NULL);
		goto err;
	}

	vitastick_driver_activated = 1;

	EXIT_SYSCALL(state);
	return 0;

err:
	EXIT_SYSCALL(state);
	return ret;
}

int vitastick_stop(void)
{
	unsigned long state;

	ENTER_SYSCALL(state);

	if (!vitastick_driver_activated) {
		EXIT_SYSCALL(state);
		return VITASTICK_ERROR_DRIVER_NOT_ACTIVATED;
	}

	ksceKernelSetEventFlag(usb_event_flag_id, EVF_DISCONNECTED);

	ksceUdcdDeactivate();
	ksceUdcdStop(VITASTICK_DRIVER_NAME, 0, NULL);
	ksceUdcdStop("USBDeviceControllerDriver", 0, NULL);
	ksceUdcdStart("USBDeviceControllerDriver", 0, NULL);
	ksceUdcdStart("USB_MTP_Driver", 0, NULL);
	ksceUdcdActivate(0x4E4);

	vitastick_driver_activated = 0;

	log_flush();

	EXIT_SYSCALL(state);
	return 0;
}

void _start() __attribute__((weak, alias("module_start")));

int module_start(SceSize argc, const void *args)
{
	int ret;

	log_reset();

	LOG("vitastick by xerpi\n");

	usb_thread_id = ksceKernelCreateThread("vitastick_usb_thread", usb_thread,
					       0x3C, 0x1000, 0, 0x10000, 0);
	if (usb_thread_id < 0) {
		LOG("Error creating the USB thread (0x%08X)\n", usb_thread_id);
		goto err_return;
	}

	usb_event_flag_id =  ksceKernelCreateEventFlag("vitastick_event_flag", 0,
						       0, NULL);
	if (usb_event_flag_id < 0) {
		LOG("Error creating the USB event flag (0x%08X)\n", usb_event_flag_id);
		goto err_destroy_thread;
	}

	ret = ksceUdcdRegister(&vitastick_udcd_driver);
	if (ret < 0) {
		LOG("Error registering the UDCD driver (0x%08X)\n", ret);
		goto err_delete_event_flag;
	}

	ret = ksceKernelStartThread(usb_thread_id, 0, NULL);
	if (ret < 0) {
		LOG("Error starting the USB thread (0x%08X)\n", ret);
		goto err_unregister;
	}

	vitastick_driver_activated = 0;
	vitastick_driver_registered = 1;

	LOG("vitastick started successfully!\n");

	return SCE_KERNEL_START_SUCCESS;

err_unregister:
	ksceUdcdUnregister(&vitastick_udcd_driver);
err_delete_event_flag:
	ksceKernelDeleteEventFlag(usb_event_flag_id);
err_destroy_thread:
	ksceKernelDeleteThread(usb_thread_id);
err_return:
	return SCE_KERNEL_START_FAILED;
}

int module_stop(SceSize argc, const void *args)
{
	ksceKernelSetEventFlag(usb_event_flag_id, EVF_EXIT);

	ksceKernelWaitThreadEnd(usb_thread_id, NULL, NULL);
	ksceKernelDeleteThread(usb_thread_id);

	ksceKernelDeleteEventFlag(usb_event_flag_id);

	ksceUdcdDeactivate();
	ksceUdcdStop(VITASTICK_DRIVER_NAME, 0, NULL);
	ksceUdcdStop("USBDeviceControllerDriver", 0, NULL);
	ksceUdcdUnregister(&vitastick_udcd_driver);

	log_flush();

	return SCE_KERNEL_STOP_SUCCESS;
}
