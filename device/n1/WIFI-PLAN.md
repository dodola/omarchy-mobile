<!-- SPDX-License-Identifier: MIT -->
# N1 Wi-Fi bring-up

**Status, 2026-10-28, kernel `5.10.110-omamobile-n1`:** Wi-Fi switching
and scanning pass five cycles plus 120 seconds of fresh-SSH checks with
NetworkManager address randomization disabled. The unchanged driver's MAC-change
path previously deadlocked; the configuration avoids that trigger rather than
repairing the kernel. Association, an assigned address, DNS and HTTPS over `wlan0` now pass.
Offline automatic loading remains gated. All physical storage is read-only;
this read-only NBD root with tmpfs overlay is not an internal-root installation.

## Observed cause and source match

The initial development root intentionally blacklisted `bcmdhd` and
`dhd_static_buf` until wireless bring-up. It also lacked the board's firmware.
There was no Wi-Fi PHY, and NetworkManager had no Wi-Fi device. The owner's
report that the switch could not enable Wi-Fi therefore matched the device
state, rather than indicating a new touch failure.

Read-only checks found PCI vendor/device `14e4:449d`, with no bound driver.
The board DT identifies `wifi_chip_type = "ap6275p"`. The owner's supplied GPL
kernel maps PCI ID `449d` to BCM43752 and its revision-2 PCIe firmware to
`bcm43752a2_pcie_ag`, with default module name `ap6275pr3`. The matching kernel
module was already installed and declares dependency `dhd_static_buf`.

Source references in the owner's unchanged checkout:

- `kernel-5.10/arch/arm64/boot/dts/rockchip/rk3588s-firefly-aio-3588sg.dtsi`
- `kernel-5.10/drivers/net/wireless/rockchip_wlan/rkwifi/bcmdhd/include/bcmdevs.h`
- `kernel-5.10/drivers/net/wireless/rockchip_wlan/rkwifi/bcmdhd/dhd_config.c`

Four matching files were copied privately from the owner's vendor source:
`fw_bcm43752a2_pcie_ag.bin`, `clm_bcm43752a2_pcie_ag.blob`,
`nvram_ap6275p.txt` and `nvram_ap6275pr3.txt`. Their complete SHA-256 manifest and
transfer/readback checks remain off-repository. No calibration, radio partition,
OTP, PCI BAR or raw register access was used. Firmware is downloaded to the
wireless controller's volatile RAM, not flashed to storage.

## Executed RAM trial and evidence

**Risk:** loading a hardware driver can lose USB or hang the kernel. Before
loading, a continuous kmsg stream to the host was active and all physical block
RO flags and the guardian's open watchdog FD were checked. A 90-second systemd
dead-man timer would stop the guardian, retaining its watchdog FD and allowing
hardware reset. After loading, fresh authenticated SSH and storage RO checks
passed before canceling the timer.

The verified files were placed under `/vendor/etc/firmware` in the RAM overlay.
The executed load was:

```sh
modprobe bcmdhd \
  firmware_path=/vendor/etc/firmware/fw_bcm43752a2_pcie_ag.bin \
  nvram_path=/vendor/etc/firmware/nvram_ap6275p.txt \
  clm_path=/vendor/etc/firmware/clm_bcm43752a2_pcie_ag.blob
```

**Pass:** `modprobe` returned success; `phy0`, `wlan0` and the Wi-Fi P2P device
appeared. NetworkManager reports Wi-Fi `enabled` and `wlan0` disconnected, ready
for association. Software/hardware rfkill flags are zero. An explicit rescan
returned 21 entries without publishing network names or addresses. Systemd has
zero failed system units; the guardian remains running and physical storage RO.

The private kernel log confirms firmware download, successful opening of the
30,993-byte CLM blob and firmware `18.35.387.23.146`, CLM `9.9.12_SS`. A final-path
debug message prints the CLM directory without its slash, but the actual
request-firmware path successfully opens the blob. This was not a missing-CLM
failure. The optional `config.txt` was absent; RTT capability returned unsupported
and some netdevice notifier messages reported null wdev. These messages did not
prevent registration or scanning; they are not a verified association failure.

**Stop:** mismatched chip/module/kernel or firmware digest, writable physical
storage, stopped guardian, missing host log or loss of fresh USB SSH. Do not
reload or probe an unresponsive endpoint. **Rollback:** the experiment changes
only RAM; return to the guarded diagnostic on restart. Do not depend on module
unload or ordinary shutdown until those paths are tested.

## Subsequent deadlock and next gate

At approximately 1085 seconds uptime, NetworkManager closed the primary
interface after an address change. Captured blocked-task stacks show:

- NetworkManager in `dhd_stop+0x358`, called through `__rtnl_newlink`, waiting
  for the driver's interface mutex while holding RTNL.
- A deferred worker in `dhd_set_mac_addr_handler+0xac`, waiting for `rtnl_lock`
  after taking `dhd_net_if_lock_local`.
- wpa_supplicant and a new sshd session subsequently blocked on netlink.

The matching GPL source in `bcmdhd/dhd_linux.c` takes the driver mutex before
RTNL in the address worker, while the interface close path takes them in the
opposite order. The firmware had loaded successfully before the failure.
Initial scans and fresh SSH therefore do not establish connection stability.
Do not send further commands to the blocked endpoint; a physical restart is
required to return to the guarded diagnostic.

The private host root staging retains verified firmware and module parameters,
but `/etc/modules-load.d/n1-wifi.conf` was removed and both bring-up blacklists
restored. The immutable exported root image predates Wi-Fi staging and does not
automatically load this driver. No new persistent image has been installed.

Next trial: configure NetworkManager scan randomization off and preserve the
hardware address for connections before manually loading the unchanged driver.
This tests avoidance of the observed trigger, not a repaired kernel mutex path.
Keep host kmsg streaming and an independent watchdog reset deadline active for
the complete scan/open/close regression. Require repeated fresh SSH, no blocked
tasks, scans and physical storage RO; then verify owner-selected association,
address assignment, DNS and Internet. Do not print network names, addresses,
credentials or connection profiles. A source lock-order correction needs a
separate build and hardware regression before default loading is considered.

## Offline lock-order candidate (not installed)

The matching unstripped object resolves the blocked `dhd_stop+0x358` to the
interface-lock cleanup loop near line 8835, and the address worker's `+0xac` to
the MAC update near line 3562. A private source copy moves the worker's RTNL
acquisition before `dhd_net_if_lock_local` and releases RTNL after the interface
mutex at the common exit, so early exits also release it. The supplied vendor
checkout is unchanged. No register access or DT change is involved.

The external module builds against the running kernel's explicit output with
`W=1`; an unchanged source copy was built with the same flags. Both emit 597
vendor warnings, with no new warning text in the candidate. Strict checkpatch
reports zero errors, warnings or checks. These are build/style results only:
the candidate has not been loaded or tested on hardware. Do not promote it into
a default root based on these checks.

## Configuration workaround result (2026-10-28)

Before loading the unchanged driver into a fresh systemd RAM root, the following
NetworkManager configuration was reloaded. It applies only to `wlan0`:

```ini
[device-n1-wifi]
match-device=interface-name:wlan0
wifi.scan-rand-mac-address=no

[connection-n1-wifi]
match-device=interface-name:wlan0
wifi.cloned-mac-address=preserve
```

A host kmsg stream and independent guardian-stop/reset timer stayed active
through the entire regression. Five Wi-Fi off/on cycles, each followed by an
explicit rescan, returned respectively 25, 23, 24, 21 and 21 entries. Every cycle
passed fresh authenticated USB SSH and physical storage RO checks. Eight further
checks at 15-second intervals passed. The captured kernel log contains no
blocked-task report or MAC-change worker execution in this trial. This establishes
the observed trigger was avoided; it does not establish arbitrary MAC changes
are safe or that association/Internet access works; those checks are recorded separately below.

Firmware, module parameters and this policy are prepared in private host root
staging. Both module blacklists remain and automatic loading stays disabled;
the currently exported immutable root predates these changes. The active fix is
RAM-only. An independent ten-minute reset timer covers owner-assisted connection
testing; cancel it only after fresh SSH/storage checks, or after ending the trial
while responsive. Do not leave it to reset an otherwise healthy idle device.

## Association and Internet result (2026-10-28)

Owner-assisted network selection brought `wlan0` to NetworkManager state 100.
A global IPv4 address exists, DNS resolution succeeds, and an HTTPS request
explicitly bound to `wlan0` succeeds with normal TLS verification. A subsequent
fresh authenticated USB SSH repeated the HTTPS and all-physical-storage RO
checks. The guardian remains alive and feeding the hardware watchdog. The log
contains no blocked-task report, MAC worker entry or kernel panic in this trial.
Both test reset timers were stopped after successful fresh checks.

This verifies the current RAM session can use Wi-Fi for Internet access. It does
not fix arbitrary MAC changes in the vendor driver or establish long-duration,
suspend/resume or boot-persistent operation. The connection profile and password
stay private in the RAM overlay and are not exported into repository artifacts.
The prepared host policy and firmware still need an independently verified root
image and an approved persistent installation before claiming restart durability.
