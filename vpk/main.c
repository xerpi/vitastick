#include <stdio.h>
#include <psp2/ctrl.h>
#include <psp2/power.h>
#include <psp2/touch.h>
#include "vitastick_uapi.h"
#include "debugScreen.h"

#define printf(...) psvDebugScreenPrintf(__VA_ARGS__)

static void wait_key_press();

int main(int argc, char *argv[])
{
	int ret;

	psvDebugScreenInit();

	printf("vitastick by xerpi\n");

	ret = vitastick_start();
	if (ret >= 0) {
		printf("vitastick started successfully!\n");
	} else if (ret == VITASTICK_ERROR_DRIVER_ALREADY_ACTIVATED) {
		printf("vitastick is already active!\n");
	} else if (ret < 0) {
		printf("Error vitastick_start(): 0x%08X\n", ret);
		wait_key_press("X", SCE_CTRL_CROSS);
		return -1;
	}

	scePowerSetArmClockFrequency(111);
	scePowerSetBusClockFrequency(111);
	scePowerSetGpuClockFrequency(111);
	scePowerSetGpuXbarClockFrequency(111);

	wait_key_press("START + SELECT", SCE_CTRL_START | SCE_CTRL_SELECT);

	scePowerSetArmClockFrequency(266);
	scePowerSetBusClockFrequency(166);
	scePowerSetGpuClockFrequency(166);
	scePowerSetGpuXbarClockFrequency(111);

	vitastick_stop();

	return 0;
}

void wait_key_press(const char *key_desc, unsigned int key_mask)
{
	SceCtrlData pad;

	printf("Press %s to exit.\n", key_desc);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
	sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
	sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);

	SceTouchData touch_old[SCE_TOUCH_PORT_MAX_NUM];
	SceTouchData touch[SCE_TOUCH_PORT_MAX_NUM];

	while (1) {
		memcpy(touch_old, touch, sizeof(touch_old));
		int i;
		uint8_t triggers = 0;

		/* sample both back and front surfaces */
		sceTouchPeek(SCE_TOUCH_PORT_BACK, &touch[SCE_TOUCH_PORT_BACK], 1);

		/* print every touch coordinates on that surface */
		for (i = 0; i < touch[SCE_TOUCH_PORT_BACK].reportNum; i++) {
			if (touch[SCE_TOUCH_PORT_BACK].report[i].x < 800 && touch[SCE_TOUCH_PORT_BACK].report[i].y > 400)
				triggers |= (1 << 0);
			if (touch[SCE_TOUCH_PORT_BACK].report[i].x > 800 && touch[SCE_TOUCH_PORT_BACK].report[i].y > 400)
				triggers |= (1 << 1);
			if (touch[SCE_TOUCH_PORT_BACK].report[i].x > 800 && touch[SCE_TOUCH_PORT_BACK].report[i].y < 400)
				triggers |= (1 << 2);
			if (touch[SCE_TOUCH_PORT_BACK].report[i].x < 800 && touch[SCE_TOUCH_PORT_BACK].report[i].y < 400)
				triggers |= (1 << 3);
		}

		upload_trigger_state(triggers);
		sceCtrlReadBufferPositive(0, &pad, 1);
		if ((pad.buttons & key_mask) == key_mask)
			break;
		sceKernelDelayThreadCB(200 * 1000);
	}
}
