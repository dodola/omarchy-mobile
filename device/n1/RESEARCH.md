<!-- SPDX-License-Identifier: MIT -->
# N1 native boot research

Status, 2026-10-28: the owner-authorized boot-only update is verified by a
complete SHA-256 readback from native `5.10.110-omamobile-n1`. USB networking,
SSH, the storage guard and watchdog pass. The Arch/Omarchy root runs through a
read-only NBD image with RAM-only changes. The full systemd/uwsm desktop,
application launch, display/touch and Wi-Fi Internet access now pass, with
software rendering and Wi-Fi address randomization disabled. Repeatable
standalone boot and persistent root integration remain incomplete. N1 stays
outside the existing Pixel installer and flash allowlists.

## Evidence and scope

The deployment session's read-only capture reports model `N1`, Android 12,
kernel `5.10.110`, product `rk3588s_firefly_aio_3588sg_mipi`, and compatible
`rockchip,rk3588s-firefly-aio-3588sg`, `rockchip,rk3588`.
The partition layout is non-A/B. The live display is a two-lane DSI panel with
480 × 800 timing and `simple-panel-dsi`; touch uses `goodix,gt9xxx` at address
0x5d. The DT model's EDP wording does not describe the active display route.
These are observations, not proof that the public reference board firmware can
boot this device. Identifying captures and proprietary images remain off-repo.

The owner supplied an Android/vendor source tree, referred to below as
`<VENDOR_SOURCE>`. The inspected U-Boot checkout is commit
`c61cb6899b8874e321208d144c0d3fb03deaa74d`; the referenced boot-source files had
no tracked modifications. There is no generated U-Boot `.config` at its root.
Its source configuration is not evidence of the configuration flashed on N1.

## Board support and the display

The public Firefly Linux BSP contains an AIO-3588SG Linux DTS whose compatible
matches the captured compatible. It includes the board, camera, MIPI101 and eDP
NV156FHM-T06 descriptions together. The MIPI description specifies 800 × 1280,
four lanes and Himax touch; the eDP description specifies 1920 × 1080. Those
settings differ from the live N1. This establishes a board-family reference,
not a bootable N1 image. Both source files are GPL-2.0+ OR MIT.
Sources: [Linux board DTS](https://github.com/Firefly-rk-linux/kernel/blob/rk3588/firefly/arch/arm64/boot/dts/rockchip/rk3588s-firefly-aio-3588sg-mipi101-M101014-BE45-A1.dts),
[MIPI description](https://github.com/Firefly-rk-linux/kernel/blob/rk3588/firefly/arch/arm64/boot/dts/rockchip/rk3588s-firefly-aio-3588sg-mipi101-M101014-BE45-A1.dtsi),
[eDP description](https://github.com/Firefly-rk-linux/kernel/blob/rk3588/firefly/arch/arm64/boot/dts/rockchip/rk3588s-firefly-aio-3588sg-edp-NV156FHM-T06.dtsi).

The local `kernel-5.10/arch/arm64/boot/dts/rockchip/rk3588s-firefly-aio-3588sg.dts`
has the same dual-license header and comments out the eDP include. The supplied
historical 720 × 720 LCD snapshot is not the current screen and must not be
selected from its filename. The exact current Android DT overlay and panel
driver changes must be carried through the initial Linux experiment.

At research time the upstream Linux Rockchip DT build list contains Firefly
ITX-3588J, ROC-RT and ROC-PC targets, but no AIO-3588SG or N1 target. Substituting
one of those DTBs is unsupported. Upstream has an RK3588 DSI2 driver, so a blanket
claim that mainline cannot drive RK3588 DSI is also incorrect; panel, PHY,
regulator, GPIO, touch and board descriptions still need to be matched and
validated. Sources: [GPL-2.0 DT build list](https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/rockchip/Makefile),
[GPL-2.0+ DSI2 driver](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/rockchip/dw-mipi-dsi2-rockchip.c).

## Removable-media boot: source evidence, not a tested procedure

The following references are source-relative to `<VENDOR_SOURCE>/u-boot` and
are GPL-2.0+ source, examined read-only:

- `include/configs/rockchip-common.h:61` defines SD `mmc1` before eMMC `mmc0`.
  Its `RKIMG_DET_BOOTDEV` at line 145 chooses SD only after `rkimgtest mmc 1`
  recognizes an IDB. A filesystem containing only `extlinux.conf` is therefore
  not sufficient to select SD through this path.
- `cmd/rkimgtest.c:29` reads IDB blocks 64–65. A legacy IDB whose update tag is
  zero adds `sdfwupdate` or `usbfwupdate` to the command line. New IDB signatures
  also pass the image test. Do not modify raw IDB fields on a guess.
- `arch/arm/mach-rockchip/board.c:365` reads the removable medium's BCB update
  message and can add firmware-update arguments. An upgrade card can reach an
  updater that writes internal storage; removable media alone does not prove
  eMMC is preserved.
- `include/configs/rockchip-common.h:166` selects boot policy conditionally.
  Only the unsigned fallback tries Android, FIT, Rockchip and then
  `distro_bootcmd`. The latter scans extlinux and boot scripts in `/` and
  `/boot/` (`include/config_distro_bootcmd.h:325`). Stock Android can boot before
  that fallback is reached.
- `arch/arm/mach-rockchip/board.c:242` implements USB-disk image selection only
  under `CONFIG_ROCKCHIP_USB_BOOT`. Its presence in source does not establish
  that the installed loader enables it.
- `configs/rk3588_defconfig:32` sets `CONFIG_BOOTDELAY=0`; a usable, interruptible
  UART console remains unverified. No persistent `saveenv` is needed for a
  prospective one-shot manual loading experiment.

Upstream U-Boot documents SD boot-image preparation and RAM `booti` with kernel,
initrd and DTB. These demonstrate available mechanisms, not N1 compatibility.
The RK3588 example also depends on DDR initialization and BL31 artifacts; keep
binary firmware outside this repository.
Sources: [GPL-2.0+ Rockchip guide](https://github.com/u-boot/u-boot/blob/master/doc/board/rockchip/rockchip.rst),
[GPL-2.0+ booti documentation](https://github.com/u-boot/u-boot/blob/master/doc/usage/cmd/booti.rst).

## Temporary fastboot boot: promising, still gated

In the supplied GPL U-Boot source, `drivers/usb/gadget/f_fastboot.c:1714` sends
the downloaded RAM image to `do_bootm`; command registration at line 2339
includes `boot`. The RK3588 defconfig sets a 112 MiB download buffer at
0x00c00800. This is source support, not a successful device test.

For an Android-format image, the traced path is `cmd/bootm.c:95` →
`arch/arm/mach-rockchip/board.c:895` (`board_do_bootm`) → DT population and
`android_image_memcpy_separate` → `common/android_bootloader.c:294`
(`android_bootloader_boot_kernel`) → `do_bootm_states`.
This path does not enter the full Android partition-selection/AVB verification
flow. The inspected callback and these entry points do not themselves issue a
BCB, rollback-index or slot-metadata write; `bootm_board_start` reads BCB.
That is not a complete no-write proof for every board hook or the booted kernel.

Image packaging is a material blocker: `boot_rkimg.c:616`
(`rockchip_ram_read_dtb_file`) expects Rockchip resource-image contents after
the aligned kernel and ramdisk when `CONFIG_ROCKCHIP_RESOURCE_IMAGE` is enabled.
Pixel boot-image packaging must not be reused blindly. Inspect the owner's
known-good image layout and source-generated configuration offline first.
Entering fastboot and executing `boot` remain human-approved boot operations.

## Boot-mode correction from the actual device (2026-10-27)

**The earlier source-only entry recommendation was wrong.** Do not use a
RESTART2 reason merely because its name looks like the desired mode. Compare
its numeric value in the **live** DT against U-Boot's constants.

The supplied source's `rk3588s.dtsi` says `bootloader = BOOT_BL_DOWNLOAD` and
`fastboot = BOOT_FASTBOOT`. However, the captured live Android DT and the DT
read from the actual recovery kernel both have:

| Kernel restart reason | Live DT value | U-Boot meaning |
| --- | --- | --- |
| `fastboot` | `0x5242c303` | `BOOT_RECOVERY` |
| `recovery` | `0x5242c303` | `BOOT_RECOVERY` |
| `bootloader` | `0x5242c309` | `BOOT_FASTBOOT` |
| `loader` | `0x5242c301` | `BOOT_LOADER` |

The constants are independently defined in the GPL sources
`u-boot/arch/arm/include/asm/arch-rockchip/boot_mode.h` and
`kernel-5.10/include/dt-bindings/soc/rockchip,boot-mode.h`.
The live values were read through the device-tree filesystem, never hardware
registers. A source-only reconstruction had missed this live/source difference.
The origin of that difference has not been established.

One human-approved test uploaded the previously compiled helper, verified its
SHA-256, and requested direct `RESTART2("fastboot")`. It entered Android
Recovery, exactly consistent with `0x5242c303`; it did **not** establish U-Boot
fastboot capability. The human confirmed the Recovery menu. Recovery kernel
is `5.10.110`. Its ADB identity differs from Android's; the single endpoint was
reidentified using model N1 and board compatible before further read-only
checks. Recovery shell uid is 2000 and `su` is unavailable. `/proc/cmdline`
was permission-denied and `/tmp/recovery.log` could not be collected. No root
bypass, flash, erase or second reboot was attempted.

The corrected candidate is the ordinary `bootloader` restart reason. From
Recovery, `adb reboot bootloader` can request it via Android's normal reboot
path; `setup_boot_mode` in U-Boot maps `BOOT_FASTBOOT` to `fastboot usb 0`.
This is now consistent with the **recovery kernel's live mapping**, but that
second reboot was subsequently authorized and tested successfully. Ordinary
`adb reboot bootloader` reaches the responsive RK3588 Fastboot endpoint.

The Apache-2.0 Android source also matters: `system/core/init/reboot.cpp`
writes the BCB for a `bootloader` request. A `fastboot` request instead routes
to recovery/fastbootd, with its own BCB update. Do not describe these standard
reboots as partition-write-free. The owner subsequently authorized necessary
deployment reboots without repeated confirmation. Never manually write `misc`
or restore an old BCB to work around the current state.

Entering Recovery can itself update BCB/cache as part of recovery's normal
startup (`bootable/recovery/recovery_main.cpp` and `recovery.cpp`). Thus the
first helper did not explicitly write a partition, but the session cannot
claim that every internal partition stayed unchanged across the reboot.

## Historical boot experiments and current remaining gates

Read-only boot-chain backups were collected and checked against second reads.
The owner supplied a complete factory image, confirmed Maskrom access and chose
the development board's internal storage. Recovery readiness is no longer a
prerequisite requested by the owner. The owner subsequently granted a named
exception for N1 boot-only deployment writes and necessary original-boot
restoration. A 64 MiB diagnostic boot image was written through rooted Android,
after a full RAM transport hash test, and its full partition readback matched.
No userdata root image has been written. Android's ordinary reboot/recovery paths can still change startup
metadata as described above.

The live revision-1 display/touch settings match the supplied product's
`dt-dvt-overlay.in`. The vendor kernel and modules built off-device as
`5.10.110-omamobile-n1`; the RAM diagnostic has an MMC-aware read-only guard,
public-key-only SSH, a watchdog overlay and an unconfirmed-boot timer. The
guard's host tests and ARM SSH authentication test passed. The first native
boot passed fresh SSH physical-storage read-only assertions. Its host check
incorrectly required WATCHDOG_SYSFS, which the stock kernel disables; without
confirmation, the 120-second deadman returned to responsive Fastboot. The
check now inspects the running watchdog feeder and its open device descriptor.
An additional MMC boot through Fastboot `continue` reached the initramfs and
reported guarded storage and SSH readiness. The host network manager removed
the manually assigned USB address, preventing confirmation before that timer
expired. These are RAM-root results, not a completed desktop installation.

Offline inspection of the actual backed-up U-Boot confirms the download address
`0x00c00800` and an additional reservation-release patch absent from the supplied
source. Android-image source allocation still conflicts with its 4 MiB kernel
destination. Legacy multi images avoid that Android-specific allocation.
Source and binary inspection also established that the kernel must remain at
the loader's fixed 4 MiB address, and that an in-place ARM64 DT must be 8-byte
aligned. Earlier 64 MiB and misaligned trials were invalid for those reasons.
The corrected new-kernel trial still failed to expose Linux USB; a corrected
stock-kernel control is being tested. A Fastboot `OKAY` only acknowledges the
command, before the boot callback executes. It is not boot evidence.

The host Fastboot CLI wraps non-Android input, so raw legacy trials use a
serial-pinned standard download/boot USB exchange. Failed trials have left a
cached Fastboot descriptor with no responsive protocol endpoint; stop target
commands at that point and request a physical restart. The owner has no debug
UART. Both power-cycle and subsequent warm-reset pstore captures contained the
new stock startup log, without the failed diagnostic kernel's trace. A built-in
USB CDC console is being prepared to obtain logs before initramfs execution.

See `BRINGUP-M1.md` for the experiment sequence, stop conditions and rollback.
The later normal-MMC kernel boot, full boot readback, guarded systemd handoff,
DRM/input, desktop, application launch and Wi-Fi Internet gates passed; see
[NORMAL-BOOT-RUNBOOK.md](NORMAL-BOOT-RUNBOOK.md) and
[WIFI-PLAN.md](WIFI-PLAN.md). The failed RAM-boot observations above are retained
as historical evidence. Current remaining gates include repeatable standalone
boot, persistent root integration and a narrowly scoped non-A/B installer. The vendor
Mali stack cannot be assumed compatible with Mesa/Hyprland. Do not add N1 to
Pixel allowlists or modify U-Boot, trust, partition tables, security, calibration,
vbmeta or Android super to bypass an unverified boot path. There is no justified
eMMC installation command yet.

Official product documentation was used as a discovery aid. Its pages did not
expose a reuse license during inspection, so it is not quoted or used as the
public source of these findings. The claims above rely on licensed source or
the session's own read-only observations.
