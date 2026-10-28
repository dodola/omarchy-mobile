<!-- SPDX-License-Identifier: MIT -->
# N1 publication checkpoint, 2026-10-28

The original N1 bring-up checkpoint starts at upstream `d7ba671`. The hardware
results below validate that package set. No package sources were changed by
that checkpoint; the later source synchronization is recorded separately below.

## Host checks

- `tests/all` passes, including `tests/n1-storage-guard` and
  `tests/n1-native-guardian`. The test entry point recognizes the native
  guardian's Python test instead of parsing it as Bash.
- N1 shell/Python syntax checks, license headers, whitespace and scans for
  device identifiers pass. No firmware, images, compiled modules, credentials
  or private captures are included.
- `tests/packages` was run. The default host Python lacks `gi`; rerunning the
  failed suites with the system Python makes `omamobile-mmd` pass. Four failures
  remain, and the same four reproduce on a clean archive of upstream `d7ba671`
  using the same system-Python environment:

  | Package | Observed failure |
  | --- | --- |
  | `omamobile-audio` | UCM HiFi-verb parsing with host `alsaucm` 1.2.6 |
  | `omamobile-sensors` | `Namespace GLibUnix not available` |
  | `omamobile-shell` | Touch QML test: `Library import requires a version` |
  | `omamobile-speaker-tuning` | `No module named 'test.test_tuning'` |

These results do not claim a passing full package suite. Complete host logs and
the clean upstream comparison remain outside the repository.

## Hardware evidence and limits

Under `5.10.110-omamobile-n1`, full boot-partition readback, all-physical-storage
read-only checks, guardian/watchdog ownership, systemd/uwsm, the Mobile home,
the real Foot launcher, display and touch passed. Wi-Fi passed five switch/scan
cycles, a further 120 seconds of fresh SSH checks, association, address
assignment, DNS and HTTPS bound to `wlan0`. Test reset timers were canceled after
fresh successful checks. The policy disables address randomization to avoid a
confirmed vendor-driver lock inversion; its source fix is not installed.

The root remains a host-exported read-only NBD image with a RAM overlay.
Standalone installation, restart persistence, hardware graphics acceleration,
long-duration Wi-Fi and suspend/resume are not verified. The standard Pixel
`tools/phone-smoke` is not reported as passing on this non-A/B N1; the
device-specific checks and their stop/rollback conditions are documented in
[BRINGUP-M1.md](BRINGUP-M1.md),
[NORMAL-BOOT-RUNBOOK.md](NORMAL-BOOT-RUNBOOK.md) and [WIFI-PLAN.md](WIFI-PLAN.md).

## Upstream source synchronization, 2026-10-28

Merged upstream `main` at `f5da1df` (26 commits since `d7ba671`) into the
hardware development branch. The only merge conflict was in `tests/all`:
both upstream's alarm-wake and per-board initramfs tests and the local storage
guard/guardian tests are retained. Hardware-port sources are unchanged. The
README retains its anonymous hardware description, tested results, upstream
attribution and references, and distinguishes the tested baseline from this
source synchronization.

`tests/all` passes with host Git 2.47.1. A system-only PATH first selected an
older Git without the `merge-tree` interface used by the kernel-policy tests;
using the normal PATH resolved that failure. `tests/packages` with system
Python reports the same four package failures listed above. All four also
reproduce on a clean archive of the newly synchronized upstream `f5da1df`
with the same host environment. A fresh Arch CI-style run was stopped during
dependency downloads; it did not produce a test result. The full package suite
is not claimed to pass.

No device commands, package updates, reboots or writes were performed for this
source synchronization. The merged package set has not been deployed or
validated on hardware; the earlier hardware results are not a claim that it
has been tested.
