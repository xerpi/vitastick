#include <stdio.h>
#include <psp2/ctrl.h>
#include <psp2/power.h>
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
	if (ret < 0) {
		printf("Error vitastick_start(): 0x%08X\n", ret);
		wait_key_press("any key", 0);
		return -1;
	}

	scePowerSetArmClockFrequency(111);
	scePowerSetBusClockFrequency(111);
	scePowerSetGpuClockFrequency(111);
	scePowerSetGpuXbarClockFrequency(111);

	printf("vitastick started successfully!\n");

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

	while (1) {
		sceCtrlReadBufferPositive(0, &pad, 1);
		if ((pad.buttons & key_mask) == key_mask)
			break;
		sceKernelDelayThreadCB(200 * 1000);
	}
}
