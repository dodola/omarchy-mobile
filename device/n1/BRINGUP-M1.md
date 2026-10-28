<!-- SPDX-License-Identifier: MIT -->
# N1: native Linux deployment gates

**Status, 2026-10-28:** native deployment is incomplete. A boot-only write,
explicitly authorized by the owner and verified by a full readback hash,
started the stock-kernel Linux RAM diagnostic with USB networking and SSH.
A further MMC boot through Fastboot `continue` produced Linux USB logs and
passed the initramfs storage guard. The new vendor kernel subsequently booted through the verified boot-only update
recorded in NORMAL-BOOT-RUNBOOK.md. The full Omarchy Mobile home subsequently
rendered in RAM. Full systemd startup and a Foot launch through the real Mobile
launcher now pass. Wi-Fi switching/scanning pass repeated checks with address randomization
disabled after diagnosing a driver lock inversion. Association, DNS and HTTPS
over Wi-Fi pass in RAM. Persistent
internal-root installation and ordinary reboot repeatability remain incomplete.
Vendor Linux `5.10.110-omamobile-n1` and its modules built with `W=1`.
N1 remains outside the Pixel installer and flash allowlists.

The vendor checkout remains unchanged at commit
`c61cb6899b8874e321208d144c0d3fb03deaa74d`. Builds use separate host directories
`<N1_WORK>/out-vendor-dt` and `<N1_WORK>/out-linux`. Firmware, captures, keys,
logs, packages and images stay outside this public repository. See
[RESEARCH.md](RESEARCH.md) and [ROOTFS-RESEARCH.md](ROOTFS-RESEARCH.md).

**Current checkpoint, 2026-10-28:** `5.10.110-omamobile-n1` is running
with authenticated USB SSH. The owner accepted full SHA-256 readback after
Linux starts because Loader masks high-LBA reads. The fixed boot-only update
completed; fresh Linux checks matched the complete 64 MiB boot hash, physical
storage read-only flags and watchdog before confirming the RAM marker.
The Arch/Omarchy root is exported read-only over NBD, mounted `ro,noload` and
covered by tmpfs. No userdata write or persistent root installation occurred.
Goodix is recognized after explicit DRM/input udev initialization. A narrowly
gated Aquamarine software-rendering patch and a proper root `/dev/shm` produced
a terminal and the full Omarchy Mobile home. Touch events and the owner confirm
input works. A boot-only update installed the guarded systemd handoff diagnostic,
and complete boot readback passed before switching the read-only NBD root's
tmpfs overlay to systemd. Fresh SSH confirmed PID 1 systemd, continued watchdog
ownership and all physical storage RO. The user manager, graphical-session
target and uwsm application scope are active; the real Mobile launcher opened
Foot. The owner confirmed desktop and touch work. Loading matching bcmdhd and
the owner's AP6275P firmware enabled Wi-Fi and scanned 21 networks, then an
address-change/close lock inversion blocked NetworkManager and new SSH. Disabling address randomization then passed five switch/scan cycles and
120 seconds of fresh SSH checks. Wi-Fi autoload remains gated in offline staging. Owner-assisted association,
address assignment, DNS and HTTPS bound to wlan0 subsequently passed; both test
reset timers were canceled after fresh SSH/storage checks. See [WIFI-PLAN.md](WIFI-PLAN.md).
See [NORMAL-BOOT-RUNBOOK.md](NORMAL-BOOT-RUNBOOK.md) and
[ROCKUSB-RESEARCH.md](ROCKUSB-RESEARCH.md).

## Goal and current authorization

Deploy a native Linux userspace and Omarchy Mobile desktop on the owner's N1.
The owner selected **internal development-device storage**, waived further
recovery planning, reported Maskrom access, and explicitly authorized necessary
reboots without repeated confirmation. Announce screen/USB changes and respect
those standing instructions. Do not require external media or another recovery
question as a prerequisite.

This board has no A/B layout. Existing slot-B tools must not be used against
its unsuffixed partitions. Prepare and validate N1-specific artifacts and a
narrow write plan before any destructive storage operation. The original
storage guard and Pixel allowlists remain unchanged. A RAM experiment protects
all physical storage read-only; it is not an installed-root storage policy.

## M0 — read-only inventory (completed)

**Risk:** low. Read Android properties, kernel configuration, live DT, block
names and sizes; no register reads or new driver loads.

**Pass, 2026-10-27, kernel `5.10.110`:**

- [V] Product `N1`, device `rk3588s_firefly_aio_3588sg_mipi`, ARM64, about
  8 GiB RAM and 64 GB eMMC. Shell is unprivileged, but `su 0 id -u` returns 0.
- [V] Live compatible is `rockchip,rk3588s-firefly-aio-3588sg`,
  `rockchip,rk3588`. Live hardware `rev` is **1**.
- [V] The active DSI1 panel is 480 × 800, two lanes, `simple-panel-dsi`;
  Goodix touch is `goodix,gt9xxx`. The EDP text in the DT model is misleading.
- [V] Kernel configuration enables the vendor Mali CSF driver, not Panthor.
  `CONFIG_FHANDLE`, `CONFIG_SYSVIPC`, `CONFIG_VT` and `CONFIG_KEXEC` are disabled.
  Stock configuration is therefore not established as suitable for the full
  desktop/systemd environment. A RAM diagnostic and a full root image have
  different requirements.
- [V] USB gadget configfs, ACM and RNDIS are built in; NCM is not. Existing
  Tensor NCM setup must not be copied unchanged.
- [V] Live `syscon-reboot-mode` has `mode-fastboot = 0x5242c303`,
  `mode-bootloader = 0x5242c309`, `mode-normal = 0x5242c300`; the
  `fd588000.syscon:reboot-mode` platform device is bound to its driver.
  These values were read from DT, **not** from hardware registers.

**Stop:** device count, board compatible or hardware revision changes.
**Rollback:** none; this gate makes no configuration changes.

## M1 — private startup backups (completed, not a complete device backup)

**Risk:** low. Read already active storage through Android's root shell and
save only to the host. Captures and images are under
`~/android/reference/n1/2026-10-27-preflight`, mode 0700; files are mode 0600.
Never copy them into this repository.

**Pass, 2026-10-27, kernel `5.10.110`:** kernel config and live DT validated;
`boot`, `dtbo`, `recovery`, `uboot`, `trust`, `baseparameter`, `security` and
`misc` copied read-only. Every image's SHA-256 matched a second read on the
device, and all ten local files passed a further manifest check.

This does **not** back up `userdata`, `super`, applications, calibration in
other locations, or the complete physical disk. These files alone are not a
validated full restore path. No restore command has been authorized.

Root `su` transport modifies binary newlines on this Android build. Binary
captures therefore used base64; large partition transfers compressed before
base64 and were decompressed and checksum-verified on the host. A timed-out
initial transfer was discarded, not treated as a backup.

The actual boot image has Android header v2, 2048-byte pages, a 40,015,880-byte
kernel, 1,350,175-byte ramdisk and a 2,592,768-byte `RSCE` second-stage resource.
The aligned payload is 43,964,416 bytes. Preserve/audit this Rockchip layout;
the project's Pixel header-v4 packing is unsuitable.

**Stop:** missing partition, invalid capture, transfer failure or any checksum
mismatch. Do not proceed on an incomplete backup.
**Rollback:** none on the device; preserve successful host backups.

## M1a — supplied factory image (completed offline)

The owner supplied `N1_Android12_MIPI_260623.img`. Its size is 1,985,421,898
bytes. The Rockchip RKFW header is dated 2026-06-23; the container's declared
regions fit the file, its trailing MD5 matches, and SHA-256 was recorded
privately. Integrity here is a file-consistency check, not authentication of
the firmware publisher. The image was extracted with the vendor's offline
`rkImageMaker -unpack` and `afptool -unpack` commands. Neither tool was used to
access the device. Format reference:
[Rockchip's GPL image reader](https://github.com/rockchip-linux/rkdeveloptool/blob/master/RKImage.cpp).

**Pass, 2026-10-27, device kernel `5.10.110`:**

- Package model is N1. All 15 named partition starts and fixed sizes match
  live MMC sysfs; the growable userdata start matches too.
- The factory and running boot resources contain identical `rk-kernel.dtb`.
  Factory DTBO matches the corresponding prefix of the backed-up partition.
- The package includes a loader, partition description, U-Boot, boot, recovery,
  DTBO, vbmeta, baseparameter, super and oem. It is a factory firmware package,
  not a userdata backup.
- Factory kernel, ramdisk, U-Boot, recovery and baseparameter differ from the
  running backups. Retain both sets; do not treat them as interchangeable or
  approve a downgrade based on the filename/container date.
- A private manifest covers all 12 extracted files. Resources and firmware
  stay outside the repository, under `<N1_WORK>/factory-260623` and
  `<N1_WORK>/image-audit`.
- A read-only check found no registered watchdog class device. Both backed-up
  and factory base DTs disable `watchdog@feaf0000`. A future RAM hardware test
  still needs its own watchdog/logging design; no watchdog has been enabled.

**Risk:** none to the device. **Stop:** corrupt package, out-of-bounds contents
or unexpected board/layout. **Rollback:** none; all extraction is on the host.
The owner's development-device instruction removes recovery planning as a
prerequisite; no restore or partition write was requested or performed.

## M2 — source and display match (completed offline, with limitations)

Let `<VENDOR_SOURCE>` be the owner's Android source root and `<N1_WORK>` a
separate host build directory. No vendor checkout files were modified.

1. Built `firefly_defconfig` and
   `rockchip/rk3588s-firefly-aio-3588sg-mipi.dtb` with Android's supplied
   clang, explicit `O=<N1_WORK>/out-vendor-dt`, `ARCH=arm64`, `LLVM=1`,
   `LLVM_IAS=1`, `CROSS_COMPILE=aarch64-none-linux-gnu-`, and `W=1`.
2. Preprocessed and compiled the product's **`dt-dvt-overlay.in`** with
   `dtc -@`, then applied it to that base DT with `fdtoverlay`.
3. Compared the merged result against the live DT: revision, DSI lane count,
   active width/height, complete panel initialization sequence and Goodix
   configuration all matched exactly.

**Risk:** none to the device. **Pass:** compilation/application succeeded and
all six comparisons matched. The historical 720 × 720 snapshot and rev3
`dt-overlay.in` are not this device's selection.

**Limitations:** the unmodified vendor DT/config build emitted 47 warnings;
this is not a warning-free validation result. `dt-validate` is unavailable on
the host. No kernel source changes have been made; checkpatch for kernel changes is
therefore not applicable. The subsequent full build is recorded below. Matching six properties is
not a full hardware or boot-safety audit.

Host ARM64 emulation now works through a QEMU binfmt handler. The signed
Arch Linux ARM root tarball and Omarchy public keyring have been verified.
An isolated ARM build environment is being assembled with required remote
package signatures. No private release key was generated or substituted.

**Stop:** any build/apply failure, changed hardware revision, or mismatch.
**Rollback:** retain logs and rebuild only the separate output directory.

## M3 — bootloader inspection (completed)

The first approved `RESTART2("fastboot")` entered Recovery because that string
maps to `BOOT_RECOVERY` in the live DT. The corrected ordinary
`adb reboot bootloader` subsequently reached U-Boot fastboot. Android's routine
BCB update is part of that authorized restart; do not manually write `misc`.

Observed product: `evb_rk3588`; bootloader: U-Boot 2017.09; maximum download:
`0x07000000`; secure: yes; unlocked: no. This does not authorize unlock or
establish secure-boot fuse state. Device identities change between modes;
pin the selected identity privately and recheck the board. Do not print IDs.

**Risk:** USB/screen interruption. **Pass:** supported variables responded.
**Stop:** an unexpected board or an unresponsive endpoint. **Rollback:** an
ordinary authorized reboot to stock when the endpoint is responsive; otherwise
ask for a physical restart. The owner has waived repeated reboot confirmations.

## M4 — kernel and RAM diagnostic (in progress)

The full vendor kernel and modules built with `W=1` in `out-linux`, release
`5.10.110-omamobile-n1`. The configuration adds systemd/namespace/IPC/file-handle,
virtual-terminal, loop/FUSE/overlay and USB ECM/NCM support. It does not turn
this vendor kernel into mainline. Vendor Wi-Fi makefiles use relative header
paths incompatible with a clean output directory; copying their headers into
that output directory allowed the build without editing vendor sources.
`modules_install` and depmod completed against the same output.

The RAM-only Alpine initramfs uses public-key-only Dropbear, USB RNDIS and ACM,
a hardware-watchdog DT overlay, a 120-second confirmation timer and an MMC-aware
fail-closed storage guard. `tests/n1-storage-guard` passes its protection and
failure cases. The actual ARM Dropbear authentication path was tested in an
isolated host container. Those checks do not prove a device boot.

Experiments and limitations:

- An Android v2 image with stock kernel, diagnostic ramdisk and Rockchip
  resources was downloaded and acknowledged by `fastboot boot`, but no Linux
  USB log or SSH appeared. Acknowledgement is not a successful boot.
- Source inspection found that the Android RAM path reserves the downloaded
  image at `0x00c00800` while allocating its raw kernel around `0x00400000`.
  The regions overlap. Compression moves the source, but the decompression
  reservation can still overlap the downloaded-image reservation. No compressed
  trial has established a working path.
- The host's `fastboot boot` automatically wraps non-Android files in an Android
  image. A uImage trial through that CLI therefore did not test raw uImage.
- A separately pinned standard fastboot USB download/boot exchange sent raw
  uImage without wrapping. The first used `0x04080000`; subsequent source/header
  inspection found stock `text_offset=0`, requiring a 2 MiB aligned base.
  That trial produced no Linux USB logs and left queries timing out. The owner
  physically restarted the board and reports no debug UART.
- The aligned 64 MiB trial and a new-kernel diagnostic at that address also
  failed to establish Linux USB. The owner observed the boot logo and physically
  restarted the board. A read-only post-restart pstore capture contained only
  the subsequent stock boot log, not a diagnostic kernel trace.
- Further GPL source inspection established two concrete errors. In
  `u-boot/arch/arm/mach-rockchip/board.c`, `board_quiesce_devices` copies from
  the environment's 4 MiB kernel address to the entry whenever those differ,
  overwriting a correctly staged 64 MiB kernel. The legacy path also retains
  the DT in place by default; the previous component layout put it at an
  address congruent to 4 modulo 8, which ARM64 Linux rejects. Relevant strings
  and address defaults are also present in the backed-up U-Boot build.
- The corrected candidate uses legacy multi load/entry `0x00400000`, pads the
  initramfs declaration/content to an 8-byte multiple and keeps DT component
  length equal to `fdt_totalsize`. Header/data CRC, DT alignment and component
  boundaries were checked offline. The legacy bootm implementation explicitly
  handles its kernel memmove overlap; the kernel destination does not reach
  the later ramdisk/DT components. No compressed image or speculative new
  protocol is needed for this correction. The corrected new-kernel trial was
  acknowledged but exposed no Linux USB, and a subsequent protocol query timed
  out. A separately packed stock-kernel control with both corrections produced
  the same result: no ACM log bytes in 180 seconds and no responsive query.
  The failure therefore also occurs with the known-running kernel; its cause
  is still unresolved.

Offline inspection of the actual backed-up U-Boot, using ordinary FIT parsing
and GNU objdump, confirms the fixed download address and a RAMBOOT patch absent
from the supplied source. It frees the Fastboot reservation before bootm; the
Android path still retains its conflicting Android source reservation. The
generic legacy path is present. These are static findings, not evidence that a
particular callback branch or Linux entry executed. The optional REA firmware
backend was unavailable; this analysis is not described as REA evidence.

After the owner's warm physical restart, the board was found in responsive
Fastboot. An authorized ordinary reboot reached Android. Pstore again contained
only the subsequent stock startup log, with no failed-trial kernel trace. A
new kernel with built-in CDC ACM/ECM and USB serial console has been built to
make logging independent of reaching `/init`. Its init reuses that gadget, with
the same physical-storage guard and no interactive serial console.

The actual Rockchip `board_fdt_chosen_bootargs` merges the downloaded DT's
`bootargs` and `bootargs_ext` into its environment before writing chosen back.
The actual/source `env_update` retains its original token snapshot while
replacing matching keys. Multiple `console=` entries can therefore consume a
later console value without appending it, depending on the original environment.
The USB-console candidate now carries only `console=ttyGS0`, with no
`bootargs_ext`; init can still write directly to `/dev/tty0`. This fixes a
diagnostic-visibility risk, not an established cause of the failed boot.

The USB-console candidate passed independent header/data CRC, component-content,
gzip-initramfs, single-console and DT-alignment checks. The kernel's built-in
CDC/serial symbols and configuration were verified; matching modules were
reinstalled. The USB-console trial ran on 2026-10-28 after the owner restarted
the board. It was acknowledged but produced zero ACM log bytes in 180 seconds,
no new USB network endpoint, and a timed-out protocol query. Target commands
stopped. After a physical restart, responsive Fastboot was returned to Android
through an authorized ordinary reboot. No Linux diagnostic boot is established.

The next proposal is in [NORMAL-BOOT-RUNBOOK.md](NORMAL-BOOT-RUNBOOK.md): a
known-running stock kernel and RAM-root diagnostic through normal MMC loading.
The image and rooted-Android writer are prepared off-device. Real-device
read-only checks and a full 64 MiB RAM-only transfer test passed. A named N1
non-A/B boot-write exception is still required before execution.

The diagnostic init now arms its hardware watchdog before the guard check and
provides serial-only logging before exposing an IP address or SSH. A failure
stops the feeder with its descriptor open; an earlier failure requests stock
boot. VT-enabled kernels can also display stage messages. These are checked
code paths, not a demonstrated functioning hardware safety net yet.

**Risk:** temporary loss of screen/USB and a possible physical reset. **Expect:**
USB product `N1 RAM probe`, log channel and authenticated SSH, then all physical
storage read-only and a functioning watchdog. **Stop:** missing protection,
watchdog/logging failure or no responsive endpoint. Do not send further commands
to a stopped device. **Rollback:** the RAM timer requests stock boot when init
is running but unconfirmed; an earlier hang may require the owner's physical
restart. No persistent image has been installed.

## M5 — internal root and desktop (pending)

The owner selected internal storage. Assemble the authentic Arch Linux ARM,
Omarchy and Mobile packages, matching modules and N1 boot integration off-device.
Keep upstream package signature verification. The published development
repository is reachable; stable returned 404 during this session. Some published
Mobile packages are older than the checkout's dependency requirements, so use
properly built local development packages where required, rather than bypassing
dependencies or fabricating release signatures.

Before persistent writes, establish a native RAM boot, USB/SSH recovery and
read-only DRM/input evidence. Then prepare a serial-pinned N1 installer with
explicit image/layout verification and a narrow partition scope. Do not touch
U-Boot, trust, partition tables, security, calibration, vbmeta or Android super
to work around an unverified boot. Account for the owner's existing userdata
when presenting any destructive write plan. Set the 480-pixel panel scale to 1;
the Pixel setup's scale 3 is unsuitable.

**Pass:** an installed native session, correct root and matching modules,
functional display/touch and verified reboot persistence. **Stop:** any failed
boot/storage gate or unresolved graphics startup. **Rollback:** document the
specific verified artifacts before persistence. The current work is not a
completed installation and does not establish N1 mainline/GPU support.

## M4 update — first native userspace (2026-10-28)

The owner explicitly authorized N1 boot-only deployment writes and necessary
original-boot restoration, with target/image/readback checks. A pinned
rooted-Android writer wrote the prepared 64 MiB padded diagnostic to the verified
`boot` block device. It uses no Fastboot flash-lock API and changes no read-only
flags. Full-partition SHA256 readback matched before the authorized reboot.

The first normal MMC boot exposed USB product `N1 RAM probe`, ACM and networking.
Fresh public-key SSH reached the diagnostic. Its physical-block read-only checks
passed; the next host assertion failed because this stock kernel does not expose
`/sys/class/watchdog/watchdog0/state`. The confirmation marker was therefore not
set, and the diagnostic's 120-second timer returned it to responsive Fastboot.
This is the first established native Linux userspace boot, not a desktop or
mainline-kernel milestone. The check now inspects the watchdog process state and
its open device descriptor instead of assuming WATCHDOG_SYSFS is enabled.

Host modem probing sent AT text into the ACM port, corrupting the serial stream;
complete logs should also be captured over SSH. A narrowly matched host udev
ignore rule was staged, but an attempted container-based rule reload returned
failure, so it is not recorded as applied at runtime. The next enumeration must
be checked. The second ordinary reboot from Fastboot has not yet exposed USB;
no further target queries were sent. The owner was asked what the screen shows.
Possible charge-animation or warm-reset causes remain hypotheses, not findings.
