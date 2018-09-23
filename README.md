# vitastick

vitastick is a plugin and an application that lets you use a PSVita as a USB controller.
It uses the UDCD (USB Device Controller Driver) infrastructure in the kernel to simulate such controller, and thus, the host thinks the PSVita is a legit USB gamepad.

**Download**: https://github.com/xerpi/vitastick/releases

**Features:**
* When the VPK is activated, it reduces the clock frequencies to reduce power consumption

**Installation:**

1. Add vitastick.skprx to taiHEN's config (ur0:/tai/config.txt):
	```
	*KERNEL
	ur0:tai/vitastick.skprx
	```
2. Install vitastick.vpk

**Usage:**

1. Open the VPK and the Vita should switch to USB controller mode

**Note**: If you use Mai, don't put the plugin inside ux0:/plugins because Mai will load all stuff you put in there...
