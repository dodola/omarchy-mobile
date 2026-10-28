<!-- SPDX-License-Identifier: MIT -->
# N1 normal-boot diagnostic

Status, 2026-10-28: the latest boot-only update is installed and its complete
partition readback verified from native `5.10.110-omamobile-n1`. The guarded
systemd/uwsm desktop, application launch, display/touch and Wi-Fi Internet
checks pass. All physical storage stays read-only at runtime; root is a
read-only NBD image with a RAM overlay. Standalone internal-root installation
remains incomplete. Earlier checkpoints below preserve the bring-up sequence;
see the final result sections for what subsequently passed.

## Candidate and evidence

The corrected legacy RAM trials, including a known-running stock kernel and a
built-in USB-console kernel, exposed no Linux USB endpoint. Acknowledged
downloads do not establish Linux entry. Normal MMC Android-v2 loading reads
components separately, avoiding the RAM path's full-image reservation conflict.

The candidate retains the known-running stock kernel and original Android-v2
load/OS metadata. It replaces the ramdisk with the tested RAM-root diagnostic
and keeps the resource DT and trailing DT identical. Base-DT changes are limited
to chosen arguments, watchdog enable/30-second timeout and the bootloader restart
mapping already observed in the live DT. The existing revision-1 DTBO is used.
The raw image is 47,943,680 bytes; zero padding makes the write exactly 64 MiB.
Its v2 ID, resource hashes, kernel, DT changes and padding passed independent
offline checks. Proprietary components remain outside this repository.

Init mounts no Android filesystem, protects all physical block devices and
provides public-key-only USB SSH. Unconfirmed healthy boots return to Fastboot
after 120 seconds. Controlled failures request Fastboot before watchdog fallback.
A hard hang before init may still require physical recovery; automatic return
is not guaranteed. This is a diagnostic, not an installed desktop.

Read-only checks completed against the pinned Android endpoint:

- Model N1, compatible, `PARTNAME=boot`, 64 MiB size and existing writable flag;
  no read-only flag was changed. Current boot SHA256 matches the verified backup.
- A complete 1 MiB current vbmeta capture matches an independent device SHA256:
  header algorithm NONE, flags 2, descriptor size 0. This is not lock-state proof.
  An earlier truncated uncompressed capture failed the checksum gate; compressed
  capture corrected it.
- The complete 64 MiB payload passed host-to-device base64 decode and SHA256
  entirely in RAM. The writer's default dry run performed no storage write.
  Missing owner acknowledgement and optimized Python are rejected before device
  access. The exact verified payload bytes are retained for the later write.

## Named exception and execution

[AGENTS.md](../../AGENTS.md) limits writes to slot B and the normal installer.
N1 is non-A/B: an explicit owner exception is required for N1 `boot`. This
session's owner granted boot-only deployment writes and necessary original-boot
restoration, with target/image/readback checks. Flags and manifests never grant
authorization. Pixel tools,
descriptors and allowlists remain unchanged.

Do not use this firmware's Fastboot flash for a strict boot-only write. Its
actual flash-lock helper and error recovery contain conditional persistent
security-state initialization writes. Its broken `getvar unlocked` is also not
lock-state proof. The rooted-Android writer avoids these APIs.

The private `normal-boot-probe/write-boot.py` checks identity, partition, size,
current boot/vbmeta hashes, both image hashes and complete payload transport.
Its only block write is fixed to `/dev/block/mmcblk0p7`, exactly 67,108,864 bytes,
with fsync and full-partition SHA256 readback. It creates no temporary userdata
image and performs no reboot or setrw operation. After explicit permission:

```sh
python3 <PRIVATE>/normal-boot-probe/write-boot.py
python3 <PRIVATE>/normal-boot-probe/write-boot.py --execute --owner-approved-boot-only
```

**Risk:** persistent boot replacement; an early hang can need Maskrom recovery.
**Pass:** verified full readback, then USB logs and fresh authenticated SSH from
the RAM root, all physical devices read-only and watchdog active.
**Stop:** any failed identity/hash/size/transport gate, uncertain write result,
missing protection or unresponsive endpoint. Do not reboot after an unverified
write. A verified write alone is not a successful boot.

## Evidence and rollback

Announce screen/USB interruption and use the already authorized ordinary reboot
after verified write success. Capture `N1 RAM probe` serial logs and verify root,
kernel, watchdog and physical read-only flags over fresh SSH before confirming
the RAM marker. Then collect read-only DRM and Goodix input evidence.

The original 64 MiB boot backup is retained off-repo and checksum verified. While
Android is still running, the writer's `--restore` mode can restore and verify
it under explicit boot-restoration authorization. That Android-only command is
not a recovery path after the diagnostic has booted or hung. The owner confirmed
Maskrom and a factory image. A boot-only Maskrom restoration must recheck device,
GPT and the captured extent (start LBA 51200, 131072 sectors of 512 bytes) before
writing. This recovery path has not been hardware-tested. Do not substitute a
full-firmware, security-state or partition-table write without separate authority.

## New-kernel update checkpoint (2026-10-28)

The new vendor kernel has systemd-required configuration and an early built-in
USB console. Its normal-MMC Android-v2 candidate passed independent component
unpacking and resource/trailing-DT equality checks. It remains off-device.
The current stock diagnostic was captured again in full and independently
matched the complete device SHA-256; GPT header/entry CRCs and the boot extent
match Linux sysfs. The native storage guard has not been weakened.

An authorized restart to `loader` reached the audited U-Boot Rockusb endpoint.
A host tool's read-only preflight passed eMMC type, capacity, full low-LBA GPT
comparison and the readable 7 MiB boot prefix. No Rockusb write has occurred.
See [ROCKUSB-RESEARCH.md](ROCKUSB-RESEARCH.md) for its actual 32 MiB read mask.
Neither the default dry run nor protocol self-tests write storage or reset.

The boot-only update required full SHA-256 verification after
the new Linux boots, because Rockusb cannot return the entire boot partition.
Its internal per-command additive readback checksum is not a substitute.
The owner explicitly answered “继续” to that concrete change in readback
sequence. The fixed boot-only writer then completed all 256 commands with
accepted additive checks and requested normal boot. A fresh SSH session from
`5.10.110-omamobile-n1` passed the physical-storage read-only and watchdog
checks; its full 64 MiB boot SHA-256 matched the padded candidate before the
RAM confirmation marker was set. This update is now readback-verified.
A flag in the private tool never grants approval by itself.

**Risk:** persistent boot replacement and an unconfirmed update if Linux fails
to start. **Pass:** after all writes are acknowledged, fresh native SSH,
physical blocks read-only, active watchdog, exact full boot SHA-256, then RAM
confirmation. **Stop:** any preflight, protocol, parity or full-hash failure;
never retry a partially completed write automatically. **Rollback:** original
or last confirmed boot, within the same captured extent and standing boot-only
restoration authority; Loader's read mask still prevents independent full
readback until a working Linux returns. Maskrom availability is owner-reported,
not a validated recovery procedure.

## Root and graphics runtime checkpoint

The verified development root image was exported over USB using read-only NBD.
The device required `nbd0` to report read-only and the exact expected capacity,
mounted ext4 with `ro,noload`, and added a tmpfs overlay. Physical eMMC remains
read-only. Nothing has been installed in userdata. The matching new kernel's
NBD, overlay and VT support work on the device.

The non-root Hyprland test uses seatd. Initial missing udev metadata and
permissions were corrected by running the root's udev daemon and triggering
only DRM/input devices, with `SYSTEMD_IN_CHROOT=0`. Aquamarine then failed its
auxiliary EGL renderer match on display-only Rockchip. A source change gated
by primary Rockchip plus explicit software rendering was compiled natively and
loaded only into the RAM overlay. After also mounting `/dev/shm` inside that
root, a terminal and the full Omarchy Mobile home rendered at 480 × 800.
Private screenshots and monitor state confirm these runtime results; hardware
acceleration is not established. See [GRAPHICS-RESEARCH.md](GRAPHICS-RESEARCH.md).

The owner confirmed the real screen has a picture and clarified that touch
works, but tapping an application does not open it. A passive input capture
recorded 73 touch press/coordinate/release events. The actual launcher invokes
`uwsm-app`; a direct invocation in this temporary chroot fails with
`Failed to connect to user scope bus via local transport: No such file or directory`.
The chroot has no systemd user manager. This is an application/session startup
gate, not a demonstrated touchscreen hardware failure.

## Systemd handoff design (subsequently executed)

A separate boot candidate keeps the same verified kernel, DT and resource. Its
only new boot behavior is an explicit RAM request to hand the verified NBD
root overlay to systemd. Until that request it remains a guarded diagnostic
with USB SSH. The new root image carries the tested Aquamarine library,
software-rendering settings, USB network configuration and private SSH host
keys. Userdata and all other physical partitions remain read-only.

`initramfs/guardian.c` keeps open descriptors to the sysfs block directory,
devtmpfs and watchdog so it can protect storage after mount paths move and
initramfs files disappear. It has no writable partition allowlist and never
sets a read-only flag to zero. A storage check failure stops watchdog feeds.
The existing shell guard stays unchanged. Native `--check-only` passed, its
static ARM64 build passed `-Wall -Wextra -Werror`, and
`tests/n1-native-guardian` checks fault paths and saved-directory operation.
It also enforces a 120-second post-handoff host confirmation deadline through
a saved `/run` directory descriptor. Actual watchdog ownership through
switch_root was a hardware gate at this checkpoint; it later passed.

**Risk:** boot replacement, screen/USB interruption, and a possible reset if
handoff protection fails. **Expect/pass:** boot-only target and image checks,
post-boot complete SHA-256, fresh authenticated SSH, all physical blocks RO,
guardian watchdog FD open, then a verified read-only root and explicit RAM
handoff; systemd, SSH, a non-root desktop and the real app launcher must work.
**Stop:** any mismatch, missing guardian, writable physical storage, failed
root verification or loss of the diagnostic endpoint. Do not request handoff
on a failed gate. **Rollback:** leave the RAM request absent; if handoff fails,
return through the already authorized boot-only restoration path using the last
confirmed diagnostic. Do not format userdata or change GPT/security partitions.

The first restart toward Loader after the graphical RAM session stopped during
vendor device shutdown after the AV1 decoder messages. USB disappeared and the
expected Rockusb endpoint did not arrive; no new write occurred. Do not assume
Loader state or replay a write. Wait for the watchdog, then use an owner-assisted
physical restart if USB does not return. A normal restart after a graphics
session is not yet repeatable.

## Offline handoff artifacts checked (2026-10-28)

The latest systemd trial boot image passed independent unpack-and-compare checks
for the kernel, ramdisk, Rockchip resource and trailing DT, including alignment
and exact 64 MiB padding. The matching private boot writer passed its boundary,
opcode and parity self-test. Its candidate digest was refreshed only after the
initramfs archive finished; an earlier incomplete-archive pack was rejected.
Neither packing nor the self-test accesses the device.

The completed native-host ext4 root image is 13,086,228,480 bytes. All five
read-only `e2fsck` phases passed, and its full SHA-256 is recorded in the private
root manifest. Use `n1-rootfs-system-trial-native.img`; the earlier interrupted
image without `-native` is not a deployment candidate. Autologin selects the
existing Omarchy session in `/usr/local/share/wayland-sessions`, which runs
Hyprland through uwsm. At this checkpoint the trial image and boot image had not been tested together;
the successful combined run is recorded below.

The following records the installation sequence already executed. For a fresh
boot of the currently installed systemd diagnostic, start with
`system-root-trial/connect-guarded.py`, then continue at root setup (step 3).
Do not use the older BusyBox-watchdog verification helper for this image or
repeat the completed write step. Before any future boot update, independently
refresh its current-image preflight against a full verified backup; a writer
whose old-image gate refers to the prior diagnostic must reject the current one.
Never bypass that mismatch.

Historical gates, in order:

1. **Low risk, read-only:** confirm the last diagnostic with
   `normal-boot-probe/connect-new-kernel.py`, or run the private boot writer's
   default preflight if already in Loader. Expect the known target, layout and
   current boot digest/prefix. Stop on any mismatch or unresponsive endpoint;
   make no write. No rollback is needed for these checks.
2. **Persistent boot replacement:** announce the interruption, use the standing
   boot-only authority with `system-root-trial/rockusb-system-trial.py`, then
   run `system-root-trial/connect-guarded.py` promptly after normal boot. Pass
   requires the complete boot SHA-256, all physical storage RO and an active
   native guardian owning the watchdog before the RAM confirmation is set.
   Stop on failure; do not retry an uncertain write. Rollback is the last
   verified diagnostic in the same boot extent under existing authority.
3. **RAM-only root setup:** export the manifest-verified native root read-only
   at USB port 10810, export name `n1-root-system`, then stream
   `system-root-trial/setup-readonly-root.sh` to the authenticated diagnostic.
   Expect the exact NBD size, `ro,noload` lower root, tmpfs overlay, tested
   graphics-library digest and open guardian watchdog FD. Stop without creating
   the handoff request on any failure. Reboot discards the RAM overlay.
4. **Screen/session interruption:** announce the change, create only
   `/run/n1-request-systemd`, and immediately run
   `system-root-trial/verify-system.py`. Pass requires PID 1 systemd, fresh
   pinned-key SSH, all physical storage RO and the guardian still owning the
   watchdog. It confirms the RAM marker within the guardian's 120-second
   deadline. A failed handoff is not a completed deployment; its unconfirmed
   watchdog path may reset the board. Return to the diagnostic or use the
   authorized boot-only rollback if necessary.
5. **Session verification:** check the real systemd user manager and the Mobile
   launcher's `uwsm-app` path, then verify an application window appears and
   touch can operate it. A visible home alone does not pass this gate.

No userdata write, format, GPT change or security-partition operation is included
in these gates. Persistent internal-root installation remains a separate step.

## Reconnection and fresh-diagnostic restart result (2026-10-28)

After an owner-assisted restart, fresh USB SSH again confirmed
`5.10.110-omamobile-n1`, the complete last-installed boot SHA-256, all physical
storage RO and the active watchdog before setting the RAM confirmation. The
systemd trial candidate has not been written.

A restart to Loader from this fresh diagnostic, before starting graphics or
mounting the desktop root, also lost USB without reaching the pinned Loader.
The passive log ended during USB host-controller shutdown. The earlier AV1
messages therefore do not prove the hang is graphics-session-specific or locate
the exact failing driver. No write was attempted. Do not repeatedly restart an
unresponsive endpoint or guess that it has entered Loader.

Host reconnection tools now bind SSH to the USB-side host address. A fallback
blackhole route for the dedicated USB subnet prevents requests from taking the
host's default network after USB disappears. An SSH response without the pinned
USB interface and authenticated key is not evidence from N1. The last successful
confirmation included both identity and full boot digest checks.

## RAM-only restart fallback (prepared, 2026-10-28)

Fresh Linux confirmation passed again after the owner resumed. Before another
Loader restart, prepare a temporary module against the exact installed kernel.
The module registers a low-priority reboot notifier that arms a ten-second
timer only for an explicit `SYS_RESTART("loader")`. The timer invokes the
exported `emergency_restart()` interface if ordinary device shutdown hangs.
It does not initiate a reboot when loaded, access registers directly, or write
storage. Unloading before reboot cancels the timer and removes the notifier.

The supplied GPL kernel documents `emergency_restart` as safe in interrupt
context. Its ARM64 restart implementation calls the pre-restart notifiers; the
Rockchip reboot-mode notifier preserves the mode already saved by the normal
reboot notifier when the emergency command is NULL. This is a narrowly scoped
workaround, not a fix for the unidentified shutdown failure. The module is a
private bring-up artifact, not an installed service or a source-tree change.

**Offline pass:** the output kernel matches the kernel extracted from the
verified installed boot byte-for-byte. External-module `W=1` build passed;
`checkpatch --strict` reported zero errors, warnings and checks, and module
vermagic matches `5.10.110-omamobile-n1` including modversions.

**Risk:** kernel-module load and temporary screen/USB interruption; an emergency
restart can still fail. Before loading, stream kmsg to the host and verify the
existing hardware watchdog and physical-storage RO flags. Transfer the module
only to `/run`, verify SHA-256, load it, then confirm fresh SSH and RO state.
**Pass:** explicit ordinary restart to Loader followed by the pinned, audited
Rockusb endpoint; then resume the separate boot-only preflight. **Stop:** any
identity, protection, load or fresh-SSH failure, or an unresponsive endpoint.
**Rollback:** unload before requesting restart; after a failed restart use the
owner-assisted physical recovery path. This uses existing necessary-reboot
authorization and adds no storage-write scope.

## Systemd and application result (2026-10-28)

The RAM fallback module loaded successfully; fresh SSH, physical storage RO and
the live host log marker passed before the authorized Loader restart. The pinned
audited Loader appeared. The private boot writer's read-only preflight passed,
then all 256 boot-only chunks were acknowledged with parity checking. The new
guarded diagnostic booted, and fresh SSH matched the complete 64 MiB boot SHA-256
before setting the RAM confirmation. The candidate's independent unpack checks
and this hardware readback refer to the same private manifest.

The verified native root image was exported read-only on USB port 10810. Root
setup passed NBD size/RO, `ro,noload`, tmpfs overlay, library SHA-256 and guardian
FD checks. The explicit RAM handoff started systemd. Initial SSH retries ended
before sshd finished startup; a subsequent fresh check within the guardian's
deadline confirmed PID 1 systemd, all physical blocks RO, guardian watchdog
ownership and overlay root before setting `/run/n1-systemd-confirmed`.
Systemd reports `running` with zero failed system units and no failed user units.

The real user manager and graphical-session target are active, with
`wayland-wm@hyprland.desktop.service` running. `omamobile-launch app --new foot`
opened a mapped Foot window in an `app-graphical.slice` scope. This passes the
same uwsm launch path that failed in the chroot. The owner confirmed the desktop
and touch now work. Screenshots and machine-readable launch evidence remain
private. One screenshot taken too early was black; it is not used as successful
home-rendering evidence.

The serial log records a vendor `u_serial.c:680 gs_close` warning as the old
initramfs log writer closes during handoff. It did not prevent systemd, USB SSH
or the confirmed application session; the warning remains a follow-up issue.
The Wi-Fi gate is recorded separately in [WIFI-PLAN.md](WIFI-PLAN.md): five
switch/scan cycles, 120 seconds of fresh SSH checks, association, DNS and HTTPS
over wlan0 passed with address randomization disabled. The vendor MAC-change
lock defect remains; this workaround and connection profile are RAM-only.
The system still depends on the host NBD export. No userdata write or standalone
internal-root deployment is established by these results.
