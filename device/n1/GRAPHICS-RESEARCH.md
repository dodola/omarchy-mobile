<!-- SPDX-License-Identifier: MIT -->
# N1 software graphics bring-up

**Status, 2026-10-28:** a native non-root Hyprland session and the Omarchy Mobile
home rendered on N1 under `5.10.110-omamobile-n1`. This is a RAM runtime test:
the Arch/Omarchy root is NBD read-only, mounted `ro,noload`, with a tmpfs overlay.
The later full systemd/uwsm session and real launcher now pass; see
[NORMAL-BOOT-RUNBOOK.md](NORMAL-BOOT-RUNBOOK.md). Persistent-root installation
and standalone boot-to-desktop integration remain incomplete. This result does
not establish GPU acceleration.

## Source provenance

The offline `aquamarine-src` checkout is the clean upstream **v0.15.1**, commit
`f31c47a1b9d300847d8fc3108ad959448103dfc1` (BSD-3-Clause). The tested
`aquamarine-n1-software` copy has the same base and one change in
`src/backend/drm/DRM.cpp`, shown below. Upstream already contains the EVDI
`rendererRequired=false` path; that path was not invented or separately added
for this experiment. See [upstream registerGPU][aq-register] and
[upstream license][aq-license].

The development root originally contained `aquamarine 0.15.1-1`,
`hyprland 0.56.2-7`, `hyprutils 0.14.3-1` and `wayland 1.26.0-1`.
Hyprland's upstream source is v0.56.2, commit
`efb50993780079460b0cbed1363e2166a2de1d9f`. The official Omarchy
[Hyprland recipe][hypr-recipe] includes `software-renderer.patch`, SHA-256
`380bb6d61c36d6c68bcccf242131f5b259734e8570ea39b3bc519b9ed137eddd`.
It classifies software rendering from `GL_RENDERER`, rather than the display
DRM driver's name, and retains the software rendering synchronization path.
The installed package already carries it; no second Hyprland patch was needed.

## Why the auxiliary renderer failed

The captured DRM setup has a Rockchip display device on `card0`/`renderD128`
and an NPU render node on `renderD129`. The vendor Mali interface is separate.
Selecting `card0` and Mesa software rendering lets Hyprland use its GBM EGL
fallback. Aquamarine's separate DRM renderer instead requires an EGL device
whose DRM filename matches the supplied FD, which the software EGL device
does not provide. The observed failure was `Can't create renderer, no matching
devices found`, followed by `initMgpu: no renderer` and failed output commits.
Sources: [Aquamarine EGL matching][aq-renderer] and
[Hyprland's device-to-GBM fallback][hypr-egl]. `EGL_BAD_PARAMETER` during device
enumeration alone is therefore not proof that Hyprland's fallback failed.

The tested change extends the upstream display-only path only for a primary
Rockchip backend explicitly requesting software rendering:

```cpp
auto drmVerName = drmVer && drmVer->name ? drmVer->name : "unknown";
driver          = driverFromName(drmVerName);
const bool softwareRockchip = !primary && std::string_view(drmVerName) == "rockchip" && envEnabled("LIBGL_ALWAYS_SOFTWARE");
if (driver == AQ_BACKEND_GPU_DRIVER_EVDI || softwareRockchip) {
    // These explicitly selected software paths expose KMS without an EGL renderer.
    primary          = {};
    rendererRequired = false;
}
```

`drmVerName` is a C string, so its contents must be compared. The `!primary`
gate preserves secondary-GPU blitting behavior. Aquamarine's
[`envEnabled`][aq-env] requires exactly `1`. The captured patch SHA-256 is
`445b1c442aaf329b7bb39dbd740c1d3b07ec0743ea90364ecd2722b480ca58b7`.

No allocator or format-capability fabrication is needed. With this gate,
[`initMgpu`][aq-init] succeeds without the auxiliary renderer,
[`onReady`][aq-ready] skips collecting its GL formats, and
[`CGBMBuffer`][aq-gbm] permits an empty renderable-format list instead of
clipping plane formats against it. Buffer clearing already checks whether a
renderer exists. The output remains GBM scanout with software rendering;
the default allocator does not become DRMDumb ([backend allocator selection][aq-allocator]).

## Shared-memory mount and the misleading client assertion

The first patched session allocated XR24 LINEAR buffers and modeset DSI-1,
but clients reported `keymap` and `format_table` marshalling failures:
`dup failed: Bad file descriptor`. Hyprland was still alive and answering
monitor queries. Both payloads use [`allocateSHMFilePair`][hypr-shm], which
calls `shm_open`, not a new `memfd_create` flag. The device chroot had no
usable `/dev/shm` mount. Mounting tmpfs mode `1777` at the chroot's own
`/dev/shm` fixed these failures. A late mount on the outer RAM init's
`/dev/shm` did not appear in the already prepared chroot `/dev`; verify the
mount inside the actual root rather than assuming propagation.

The subsequent `Couldn't open a gbm fd`, `OpenGL.cpp:508`, `[HT]` assertion
belongs to [hyprtoolkit v0.6.0][ht-egl], not Hyprland. Toolkit obtains its DRM
node from the compositor's default DMA-BUF feedback, then constructs a GBM
allocator ([WaylandPlatform.cpp][ht-feedback]). Broken feedback can leave
its FD unset. Aquamarine's allocator error also combines a failed
`drmGetCap` with a genuine missing PRIME-export capability; that message alone
does not distinguish an invalid FD from missing hardware support.

Toolkit uses a NULL Aquamarine backend and has no GPU-selection override for
this path; `AQ_DRM_DEVICES` selects the compositor backend, while toolkit uses
the advertised feedback device ([toolkit backend][ht-backend]). No toolkit
patch was required after the shared-memory correction. Inspection of the
installed hyprutils FD routines matched the [v0.14.3 ownership implementation][hu-fd];
no FD ABI mismatch was established.

## Reproduction and packaging checkpoint

The native test built only in the RAM development root:

```sh
cmake -S /run/build/aquamarine-n1-software \
  -B /run/build/aquamarine-n1-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF
cmake --build /run/build/aquamarine-n1-build -j4
```

The resulting `libaquamarine.so.0.15.1` replaced the matching library only in
the RAM root. SONAME 14 and dependencies were checked. Its SHA-256 was
`a1e91fb6e887c002374b6bad8fc0bc9a9cf032f730836bacb66af17cfd637431`.
This records the tested artifact; independent toolchains need not produce
that same ELF hash.

Runtime used seatd, non-root execution, explicit DRM/input udev initialization
and these environment settings:

```sh
export LIBGL_ALWAYS_SOFTWARE=1 AQ_DRM_DEVICES=/dev/dri/card0
export QT_QUICK_BACKEND=software QT_SCALE_FACTOR=1 GDK_SCALE=1
export OMARCHY_PATH=/usr/share/omarchy
```

The minimal terminal session rendered first. The actual user Hyprland Lua
configuration then rendered the Mobile home with status bar, clock, apps and
search. Quickshell was launched manually with
`dbus-run-session quickshell -n -p /usr/share/omarchy/shell` because this
experiment did not run the full systemd session. Private screenshots
`n1-screen-patched.png` and `n1-full-screen.png` document those two results;
the captures and complete runtime logs remain outside this repository.

For a durable package, start from the official Omarchy
[Aquamarine recipe at b6ace05][aq-recipe]: **0.15.1-1.3**, aarch64. It uses
upstream archive SHA-256
`2f9de98c0bd1b7b1b09c576e390a2fef436449762fb334163c414f0c300296f2`
and four existing connector/teardown fixes. Preserve those sources, checksums,
dependencies and patches; append the N1 patch after them, add its checksum,
and assign a distinct local release such as `pkgrel=1.4` (makepkg accepts only
one dot in pkgrel). Use the recipe's
Release build and staged `DESTDIR` install, then install the resulting local
package through the existing development-package policy. The tested RAM build
was upstream v0.15.1 plus the N1 patch. A subsequent native makepkg build of
the combined recipe completed in RAM, but its package was not retrieved before
the Wi-Fi deadlock forced a restart; it has not been installed or runtime-tested.
The current systemd session still uses the minimal tested library. Package
integration and persistence remain separate gates in
[BRINGUP-M1.md](BRINGUP-M1.md).

[aq-license]: https://github.com/hyprwm/aquamarine/blob/f31c47a1b9d300847d8fc3108ad959448103dfc1/LICENSE
[aq-register]: https://github.com/hyprwm/aquamarine/blob/f31c47a1b9d300847d8fc3108ad959448103dfc1/src/backend/drm/DRM.cpp#L1120
[aq-renderer]: https://github.com/hyprwm/aquamarine/blob/f31c47a1b9d300847d8fc3108ad959448103dfc1/src/backend/drm/Renderer.cpp#L233
[aq-init]: https://github.com/hyprwm/aquamarine/blob/f31c47a1b9d300847d8fc3108ad959448103dfc1/src/backend/drm/DRM.cpp#L860
[aq-ready]: https://github.com/hyprwm/aquamarine/blob/f31c47a1b9d300847d8fc3108ad959448103dfc1/src/backend/drm/DRM.cpp#L1534
[aq-gbm]: https://github.com/hyprwm/aquamarine/blob/f31c47a1b9d300847d8fc3108ad959448103dfc1/src/allocator/GBM.cpp#L113
[aq-allocator]: https://github.com/hyprwm/aquamarine/blob/f31c47a1b9d300847d8fc3108ad959448103dfc1/src/backend/Backend.cpp#L159
[aq-env]: https://github.com/hyprwm/aquamarine/blob/f31c47a1b9d300847d8fc3108ad959448103dfc1/src/utils/Shared.cpp#L4
[aq-recipe]: https://github.com/omacom-io/omarchy-pkgs/blob/b6ace05ccc4b426e433714b0169b1dff3a92a12a/pkgbuilds/aquamarine/PKGBUILD
[hypr-recipe]: https://github.com/omacom-io/omarchy-pkgs/blob/ebedd694af9f5c715b9761a917fde9da2b4f2262/pkgbuilds/hyprland/PKGBUILD
[hypr-egl]: https://github.com/hyprwm/Hyprland/blob/efb50993780079460b0cbed1363e2166a2de1d9f/src/render/OpenGL.cpp#L332
[hypr-shm]: https://github.com/hyprwm/Hyprland/blob/efb50993780079460b0cbed1363e2166a2de1d9f/src/helpers/MiscFunctions.cpp#L656
[ht-egl]: https://github.com/hyprwm/hyprtoolkit/blob/f8cbaf42e1f7fdc9c7565d7be3e935f6d56c7a39/src/renderer/gl/OpenGL.cpp#L508
[ht-feedback]: https://github.com/hyprwm/hyprtoolkit/blob/f8cbaf42e1f7fdc9c7565d7be3e935f6d56c7a39/src/core/platforms/WaylandPlatform.cpp#L521
[ht-backend]: https://github.com/hyprwm/hyprtoolkit/blob/f8cbaf42e1f7fdc9c7565d7be3e935f6d56c7a39/src/core/Backend.cpp#L133
[hu-fd]: https://github.com/hyprwm/hyprutils/blob/v0.14.3/src/os/FileDescriptor.cpp
