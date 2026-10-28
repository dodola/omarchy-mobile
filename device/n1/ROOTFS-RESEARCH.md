<!-- SPDX-License-Identifier: MIT -->
# N1 development rootfs research

Research date: 2026-10-27. This covers an authentic Arch Linux ARM + Omarchy +
Omarchy Mobile userspace with the owner's Linux 5.10 kernel. It does not claim
mainline N1 support, a working display, or a completed device installation.

## Upstream binaries are available

The official `stable/aarch64/omarchy.db` was read directly over verified HTTPS.
It is Zstandard compressed, not gzip. Its SHA-256 during this investigation was
`ebd013a2335f04f9e1ac504701770d07e24969cc894865b848139e6856706b54`.
The database listed:

- `omarchy 4.0.6-1` and `omarchy-settings 4.0.6-1`;
- `omarchy-keyring 20251027-1`, architecture `any`;
- `hyprland 0.56.2-7`, architecture `aarch64`;
- `quickshell 0.3.2.r7.g2b4501a-1`, architecture `aarch64`.

Source: [official stable ARM64 database](https://pkgs.omarchy.org/stable/aarch64/omarchy.db).
Repository topology and multi-architecture packaging are documented in
[Omarchy's package repository](https://github.com/omacom/omarchy-pkgs).
Omarchy's ARM64 pacman configuration puts `[omarchy]` before Arch Linux ARM and
uses required package signatures with optional database signatures:
[upstream ARM64 pacman configuration](https://github.com/omacom/omarchy/blob/quattro/default/pacman/aarch64/pacman-stable.conf).

This removes the need to compile Hyprland or Quickshell merely because N1 is
ARM64. Install a coherent dependency transaction and record package versions;
the database and rolling upstream repositories can change after this date.

## Trust bootstrap works on an Ubuntu build host

The official package-builder source pins Omarchy's primary signing key to
`40DFB630FF42BCFFB047046CF0134EE680CAC571` and explains that its keyring package
is architecture-independent:
[official builder Dockerfile](https://github.com/omacom/omarchy-pkgs/blob/master/build/Dockerfile).

The following was actually completed, entirely off-device:

1. Download the stable `omarchy-keyring-20251027-1-any.pkg.tar.zst`, its detached
   signature, and the official package-source public keyring over HTTPS.
2. Import the public keyring into a temporary isolated GnuPG directory. Require
   a valid detached signature whose primary fingerprint matches the pinned key.
3. Extract `omarchy.gpg`, `omarchy-trusted`, and `omarchy-revoked` only after
   verification. Keep the package, signature, verification log and provenance
   manifest in private host-dependency staging. Remove the temporary GnuPG
   directory. The host's installed keyring was not changed.

The verified package SHA-256 is
`cce48c07cfc2b97e198df1bc81705db2921685032fe39f18b4ddfd84264b5967`.
The verified `omarchy.gpg` SHA-256 is
`15d6aac44df688165b2ea35fe0b23af239bbc66a6909c10a5c219e8d94b707de`.
Point `OMARCHY_KEYRING_DIR` at that private staging directory when using
`tools/lib/alarm.sh`; the build host need not itself run Omarchy.

Inside the isolated ARM root, initialize pacman's own keyring, populate
`archlinuxarm`, install these verified public keyring files under
`/usr/share/pacman/keyrings`, and populate `omarchy`. Keep
`SigLevel = Required DatabaseOptional`; do not disable remote-package signature
checks to bootstrap trust. The official builder's temporary `SigLevel=Optional`
shortcut is unnecessary when the keyring package has already been verified.

Arch Linux ARM signs its root tarballs and packages with the published builder
key `68B3537F39A313B3E574D06777193F152BDBE6A6`:
[downloads](https://archlinuxarm.org/about/downloads),
[package-signing policy](https://archlinuxarm.org/about/package-signing).
Its generic ARM64 root deliberately leaves board boot integration to the
developer: [generic installation](https://archlinuxarm.org/platforms/armv8/generic).

The `os.archlinuxarm.org` HTTPS hostname failed certificate verification in
this session. Two hosts in the official source mirrorlist successfully served
the tarball, `.md5` and `.sig` with normal TLS verification:

- `https://fl.us.mirror.archlinuxarm.org/os/ArchLinuxARM-aarch64-latest.tar.gz`
- `https://de3.mirror.archlinuxarm.org/os/ArchLinuxARM-aarch64-latest.tar.gz`

HEAD reported 829,367,415 bytes; the MD5 sidecar reported
`23eec86365b24f7913c403e8f4e8719b`. These availability checks are not signature
verification of the full tarball. The builder must still verify the downloaded
tarball's detached signature against the pinned builder key before extraction.
Source: [official mirrorlist](https://github.com/archlinuxarm/PKGBUILDs/blob/master/core/pacman-mirrorlist/mirrorlist).

## Smallest honest local package set

The existing release tools enforce useful release constraints:

- `tools/build-packages` requires the maintainer's private signing key and
  creates signed `[omamobile]` packages and database.
- `tools/build-rootfs` requires a real device descriptor, a device package,
  kernel modules and that repository. `rootfs/build-inside` insists that its
  mobile packages resolve from `[omamobile]`.
- `--allow-missing` can avoid the slow Mesa, Hyprland and libcamera builds,
  but it does not remove the release signing or device-package requirements.

A development image can instead reuse the verified ARM bootstrap/container,
then build actual PKGBUILDs as an unprivileged user and install local package
files with pacman. `LocalFileSigLevel=Optional` is already the upstream policy
for explicitly installed local build files; remote packages remain signature
required. Record that these are local unsigned development packages. Do not
generate a substitute maintainer key or fabricate a Pixel device descriptor.

The dependency-closed local package set for the current packaged mobile shell
is six packages, built with their existing recipes:

1. `neard` — small autotools/C build, required by `omamobile-apps`.
2. `omacam` — the Rust camera app; use the checksummed release and Cargo.lock.
3. `omamobile-apps` — package data/hooks, depends on both of the above.
4. `omamobile-omarchy` — scripts, patches and lock-screen PAM setup.
5. `omamobile-owl-widgets` — data from its pinned widget-store commit, with
   per-widget checksums verified by its installer.
6. `omamobile-shell` — scripts, QML plugins and configuration; depends on 3–5.

`omamobile-firstboot` is an additional small script package when its provisioning
workflow is used. No device package, `omamobile-keyring`, Pixel audio/sensor/
modem package, custom Mesa or custom Hyprland is needed just to build this
development userspace. Installing package dependencies does not establish that
N1 has working cameras, NFC, audio or radios. The local recipes are the source
of this dependency analysis: `packages/*/PKGBUILD` and their install scripts.

Arch Linux ARM currently supplies
[libcamera 0.7.2-4](https://archlinuxarm.org/packages/aarch64/libcamera) and its
IPA package, satisfying `omacam`'s `>=0.7,<0.8` dependency;
[Rust 1:1.99.1-1.1](https://archlinuxarm.org/packages/aarch64/rust) provides cargo;
[Squeekboard 1.43.1-6](https://archlinuxarm.org/packages/aarch64/squeekboard)
provides the keyboard. The source checkouts are not enough to establish that a
package exists for ARM64; these were checked against the actual ARM64 package
pages. No custom libcamera build is required for dependency satisfaction.

## Software rendering and kernel requirements

Omarchy's published Hyprland recipe already includes `software-renderer.patch`,
which detects llvmpipe/softpipe from `GL_RENDERER` even when the DRM device is a
display-only controller. The project's additional Hyprland patch changes DPMS
retry behavior, and its Mesa patches primarily add other Mali model support.
Neither is a demonstrated N1 startup requirement.
Sources: [official Hyprland recipe](https://github.com/omacom/omarchy-pkgs/blob/master/pkgbuilds/hyprland/PKGBUILD),
[software-renderer patch](https://github.com/omacom/omarchy-pkgs/blob/master/pkgbuilds/hyprland/software-renderer.patch),
local `packages/hyprland/README.md` and `packages/mesa/README.md`.

Start with signed upstream Mesa and test `LIBGL_ALWAYS_SOFTWARE=1`; Mesa
documents that variable. This does not create DRM/KMS support or fix buffer
import/scanout. Aquamarine 0.15.1 checks PRIME import, CRTC-in-vblank events,
monotonic timestamps and universal planes. Atomic and explicit timeline support
are detected; missing timeline support is not itself a universal rejection.
The live DRM driver and EGL/GBM integration remain hardware gates.
Sources: [Mesa variables](https://docs.mesa3d.org/envvars.html),
[Aquamarine DRM implementation](https://github.com/hyprwm/aquamarine/blob/v0.15.1/src/backend/drm/DRM.cpp).

The stable Quickshell package and signature were also downloaded and verified
against the pinned Omarchy key. Its `.BUILDINFO` lists Qt base 6.12.0-3 and Qt
declarative 6.12.0-1. Current ARM64 packages are Qt base 6.12.0-3 and declarative
6.12.0-2; there is no identified Qt-version mismatch. Still run
`quickshell --private-check-compat` in the assembled root; this is the check
the official package hook invokes after Qt updates.
Sources: [Quickshell recipe and hook](https://github.com/omacom/omarchy-pkgs/tree/master/pkgbuilds/quickshell),
[Qt base](https://archlinuxarm.org/packages/aarch64/qt6-base),
[Qt declarative](https://archlinuxarm.org/packages/aarch64/qt6-declarative).
Qt's raster scene-graph backend is an optional fallback, but drops ShaderEffect
and particles; llvmpipe preserves the GL rendering path and must be tested first.
Source: [Qt software adaptation](https://doc.qt.io/qt-6/qtquick-visualcanvas-adaptations-software.html).

Current ARM core metadata reports systemd 262.1-1. Upstream v262 requires kernel
5.10 or later and recommends 5.14 or later. The vendor 5.10.110 version meets
that minimum, but configuration still matters: devtmpfs, cgroups, inotify,
signalfd, timerfd, epoll, Unix sockets, sysfs, procfs and file handles are listed
requirements. A usable unified cgroup hierarchy, udev and logind session must
be demonstrated. Source: [systemd v262 requirements](https://github.com/systemd/systemd/blob/v262/README).

## Practical assembly path

1. Verify and extract the generic root tarball off-device. Use QEMU user-mode
   plus the existing container helpers, or an isolated chroot with a working
   static emulator; keep all mounts and changes inside the build root.
2. Bootstrap trusted Arch Linux ARM and Omarchy keyrings as above; configure
   stable Omarchy before ARM core/extra/alarm repositories. Perform a coherent
   package upgrade and install `omarchy`, `omarchy-settings`, base services,
   networking, a terminal, portals, Qt/Wayland and the shell's dependencies.
3. Build/install the six local packages in dependency order. Build their real
   sources and run appropriate existing checks. Include `omamobile-firstboot`
   if using its lifecycle. Save package files, hashes, source revision and a
   complete installed-package manifest in private build output.
4. For the full Omarchy application set, use the installed
   `/usr/share/omarchy/install/omarchy-base.packages`, filtered by actual ARM64
   availability as `rootfs/build-inside` already does. Record unavailable/skipped
   packages. A small desktop bring-up selection is a milestone, not proof that
   all applications or device functions work.
5. Seed the non-root user's home from the installed settings package. Use
   `omarchy-provision-user --force --first-install` as that user, then
   `omamobile-setup-user`. A session D-Bus is required by settings tools.
   Omarchy's provisioning distinguishes this offline first-install context:
   [upstream user provisioning](https://github.com/omacom/omarchy/blob/v4.0.6/bin/omarchy-provision-user).
6. Override the mobile script's Pixel-specific monitor/GDK scale of 3 to an N1
   scale of 1 after setup; otherwise a 480-pixel display becomes only 160 logical
   pixels wide. The current setup also runs `chromium --version`, so install
   Chromium before running it. Validate the profile, plugins, terminal, keyboard
   and lock credentials before enabling automatic locking.
7. Install the exact matching kernel modules and N1-specific boot/network/
   storage setup separately. Do not install a Pixel device package to obtain
   those files. Check ELF dependencies, package dependency closure, Omarchy patch
   application, Quickshell compatibility and shell configuration before boot.

The remaining blockers are runtime boot/root integration, usable DRM scanout and
EGL/GBM software rendering, session/input behavior and the user's eventual
persistent root medium. Missing upstream ARM64 Omarchy binaries, missing
Omarchy-host keyrings and mandatory custom Mesa builds are not blockers now.

## Build-session update

The full generic root tarball was subsequently downloaded from the TUNA mirror,
matched the official MD5 sidecar, and passed detached-signature verification
against the pinned Arch Linux ARM key. QEMU binfmt execution, nested ARM chroot
and public-key-only ARM Dropbear authentication were tested. The isolated build
container completed its coherent upgrade and base-devel setup with required
remote package signatures. Its Arch mirrors use verified HTTPS to TUNA.

The development Mobile database is reachable at
`https://pkgs.omamobile.org/development/aarch64/omamobile.db`; stable returned
HTTP 404. It contains the current omacam/neard/apps/widgets release packages,
but its Omarchy integration and shell packages lag the checkout. The actual
checkout recipes built local `omamobile-omarchy 0.1.12-1` and
`omamobile-shell 0.2.28-1`; these are unsigned development files, installed
explicitly through the existing local-file signature policy. This avoids
rebuilding Rust merely to obtain an already published signed camera package.
Development is placed after Omarchy/Arch repositories for this trial, so the
signed upstream graphics stack remains the selected baseline.

The native host Omarchy-integration test suite passed. The host shell suite
reached a Qt import-version failure under its available QML runtime; it is not
recorded as a passing full shell test. The failing switcher case subsequently
passed with the assembled ARM root's actual Qt 6 runtime: 10 passed, 0 failed.
`quickshell --private-check-compat` also exited successfully. These checks do
not establish that the full shell suite passed.

The desktop, local Mobile and complete upstream application-list transactions
completed. The `nvim` and `dotnet-runtime` dependency names resolve through
installed providers; they are not missing ARM64 applications. Only the upstream
rootfs skip for foreign-architecture binfmt was intentional. `pacman -Dk`
reported no database errors. The Omarchy patch hook reports 19 applied, 0 already upstream, across
18 files. The module-loading hook reports the expected absence of a running
systemd manager in the build container; device service behavior remains
untested. Offline user provisioning completed with scale 1, SSH key access and
automatic locking disabled until touch works. Laptop hardware setup leaves and
optional mise developer-tool downloads were explicitly deferred; the installed
provisioner ran the remaining setup leaves. The build-container profile checks
are not a graphical N1 session. A native filesystem export completed; final
boot/root integration and image validation remain. Package availability and
build completion do not prove runtime graphics or deployment success.
