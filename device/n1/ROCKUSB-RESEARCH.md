<!-- SPDX-License-Identifier: MIT -->
# N1 Rockusb boot-update research

Status, 2026-10-28: offline protocol and firmware audit only. This note does not
record an executed Rockusb write or grant permission. A boot-only update cannot
be declared verified from a Rockusb success response: this firmware deliberately
substitutes bytes for reads beyond 32 MiB. Full SHA-256 readback requires a
separate working Linux/Android path; that path may be unavailable after a failed
candidate boot.

## Sources and evidence limits

`<VENDOR_SOURCE>/u-boot` is commit
`c61cb6899b8874e321208d144c0d3fb03deaa74d`. References below are to its GPL-2.0+
sources. The actual backed-up U-Boot was separately inspected with GNU objdump;
addresses are link virtual addresses, with payload base `0x200000`. Private
payload, full disassembly and provenance remain outside this repository. This
is ordinary static binary analysis, not REA Evidence or runtime tracing. The
binary has known differences from the supplied source, so matches are called
out explicitly rather than assuming source equivalence everywhere.

The GPL-2.0+ official host implementation was read at commit
`304f073752fd25c854e1bcf05d8e7f925b1f4e14`:
[RKComm.h](https://github.com/rockchip-linux/rkdeveloptool/blob/304f073752fd25c854e1bcf05d8e7f925b1f4e14/RKComm.h),
[RKComm.cpp](https://github.com/rockchip-linux/rkdeveloptool/blob/304f073752fd25c854e1bcf05d8e7f925b1f4e14/RKComm.cpp).
The tool was not compiled or executed against hardware during this research.
No loader download, storage selection or device operation was performed.

## Physical addressing and the read limit

`cmd/rockusb.c:21-36` computes `blkstart = start + start_sector`, then tests
`blkstart + blkcnt > 65536`. A request above that end boundary returns the
requested sector count with a buffer filled with `0xcc`, without reading MMC.
The limit is defined in `include/rockusb.h` as `32 * 2048` sectors. Actual
`0x21112c..0x211184` contains the same addition, comparison against `0x10000`,
fill with `0xcc`, and success-count return. Below the limit it calls the
source-equivalent `blk_dread` at `0x24a7d0`.

The captured N1 `boot` extent is `[51200, 182272)` in 512-byte sectors, 64 MiB.
Only the first 14336 sectors, 7 MiB, are inside the readable region. The
mass-storage layer splits transfers into buffers; therefore a large command
may return some genuine data followed by substituted buffers. Reducing chunk
size does not enable reading higher addresses. A matching length, CSW, or
successful host exit is insufficient proof of readback.

`cmd/rockusb.c:118-125` deliberately exposes the entire selected block device
with `start_sector=0`, even though its parser accepts partition syntax. Actual
initialization `0x210e44..0x210e68` sets the zero offset and installs the limited
reader and ordinary writer. The limit uses the resulting physical address, so
partition-relative addressing would not remove it in any case.

`drivers/usb/gadget/f_rockusb.c:rkusb_cmd_process` supports LBA read `0x14`,
which becomes SCSI READ(10). Old read `0x04`, SDRAM read `0x17`, and SPI read
`0x21` are unsupported. Unknown Rockusb opcodes jump to `unknown_cmnd` before
the generic SCSI switch in `f_mass_storage.c:1806-1812`; sending native SCSI
READ(6), READ(10), or READ(12) is therefore not an alternate read interface.
Actual dispatch `0x2940d8..0x294110` translates `0x14` to `0x28`, while unknown
branches reach `0x2936a8`. No supported full-media read bypass was established.

## Minimal protocol layout

The packed CBW is 31 bytes: little-endian 32-bit signature `0x43425355`
(`USBC`), tag, byte transfer length; one-byte flags, LUN, CDB length; then a
16-byte CDB. The CSW is 13 bytes: little-endian signature `0x53425355` (`USBS`),
matching tag, residue, then status. Ordinary complete data operations require
exact lengths, matching tag/signature, zero residue and zero status. Official
host code omits some status checks; an installer must not inherit those
omissions or its unlimited `CMD_TIMEOUT=0`.

For read `0x14` and write `0x15`, CDB bytes 2-5 contain big-endian start LBA and
7-8 contain big-endian sector count. Byte length must equal count times 512.
LUN is zero, CDB byte 1 is zero, unused bytes are zero, and count must not wrap
the 16-bit field. The ordinary CDB length is 10; direction is `0x80` for read
and zero for write. Sources: `RKComm.cpp:185-240,473-517,691-731` and vendor
`f_rockusb.c:rkusb_fixup_cbwcb`. No implicit erase is needed for the ordinary
eMMC LBA write path.

## Queries before a write

These queries only construct responses from device/block metadata in the
inspected implementation. Use CDB length 6, direction `0x80`, zero unused
fields, and the indicated byte length:

- `0x01`, length 5: returns `EMMC ` for the MMC block type.
- `0x1a`, length 11: packed little-endian `u32 flash_size` in sectors,
  `u16 block_size`, then five `u8` fields for page size, ECC bits, access time,
  manufacturer and flash mask. `flash_size` is `desc->lba`; the other size
  fields are protocol metadata, not evidence of eMMC erase geometry.
- `0x2b`, length 4: little-endian media bitmask; eMMC is `1 << 1`, value 2.
- `0x1b`, length 16: chip information. It is not a unique device identity.

Sources: host `RKComm.cpp:RKU_ReadFlashID`, `RKU_ReadFlashInfo`,
`RKU_ReadStorage`, `RKU_ReadChipInfo`; vendor
`f_rockusb.c:rkusb_do_read_flash_id`, `rkusb_do_read_flash_info`,
`rkusb_do_get_storage_info`, and `arch/arm/include/asm/arch-rockchip/rk_atags.h`.
Actual `0x294138..0x294200` builds the 11-byte flash-info response from the
block descriptor. `0x293560..0x2935c8` implements the media response and value 2
for the matching MMC case.

There is no direct hardware-partition number in those query responses. Pinning
the expected USB device, requiring eMMC type, comparing the complete sector
count with the prior Linux capture, and independently comparing the genuine
low-LBA GPT header/entries and CRCs can establish that the exposed address
space matches the expected eMMC user area. Any mismatch must stop before a
write. Chip information or capacity alone is insufficient. Do not guess LUNs,
issue change-storage, or infer that boot hardware partitions are selected from
the USB product label.

## Write path and parity are bounded evidence

Actual `0x292ee4..0x29313c` translates `0x15` to SCSI WRITE(10), then the
ordinary write path reaches `0x292430`, callback `0x210c78`, and
`blk_dwrite` `0x24a844`. The reviewed path has no Fastboot lock-state API,
security-state initialization, BCB write, or extra partition write. Source
`cmd/rockusb.c:rkusb_write_sector` agrees. This focused audit does not prove
all storage-driver or firmware behavior, nor does it test recovery.

Vendor `f_rockusb.c:rkusb_do_check_parity` implements an optional write-readback
checksum. CDB bytes 9-12 hold a little-endian 32-bit value:
`(0x55aa + sum(payload little-endian u32 words)) mod 2^32`.
Zero disables the check. After writing, it directly calls `blk_dread` and
compares the additive checksum, setting phase error on failure. Actual
`0x29234c..0x29242c` confirms the direct read, initial value and 32-bit adds.
This reads past the ordinary Rockusb read limit internally but returns no data
or cryptographic digest to the host.

The checksum buffer is 1 MiB; a checked command must never exceed 2048 sectors.
The host must reject a computed zero instead of silently disabling validation.
Use CDB length at least 13 so all checksum bytes are copied; 13 is accepted by
the inspected parser, or 16 with bytes 13-15 zero. `received_cbw` copies only
the declared CDB length. The Rockusb WRITE(10) check explicitly allows bytes
9-12 (`f_mass_storage.c:2001-2008`). Official rkdeveloptool does not implement
this extension; its default ten-byte CDB cannot carry the complete checksum.
These are static findings, not a hardware-tested parameter set.

The additive checksum detects some corruption but has collisions and is not
the required full-partition SHA-256 readback. A writer using it must still
remain strictly inside the freshly verified boot extent and cannot claim the
owner's readback requirement is fulfilled solely from parity success.

The target parity implementation only rejects a zero `blk_dread` return; it
does not require that the returned sector count equals the request. Actual
`0x2923f4` confirms that test. A positive partial read could leave stale bytes
in the static buffer, so even an accepted checksum is weaker than an exact
full-command readback. Host CSW validation cannot repair this target-side
limitation. The separate full Linux SHA-256 remains mandatory.

## Reset and post-write verification

Reset is opcode `0xff`, CDB length 6, OUT direction, zero payload. CDB byte 1
is its subcode: zero/default requests normal boot; 3 requests BootROM download.
Vendor `f_rockusb.c:__do_reset` writes the boot-mode register and invokes reset.
Actual callback `0x291454..0x291498` selects `0x5242c300` for normal or
`0xef08a53c` for subcode 3, writes the register, then calls reset. It contains
no flash/BCB operation. A command ACK precedes successful boot and cannot prove
that the next kernel ran; earlier normal warm resets were not repeatable.

If the new kernel reaches a fresh authenticated session and passes the
physical-storage read-only gate, Linux can hash all 64 MiB of the boot partition
without changing its read-only state. Compare with the exact padded candidate,
not just its kernel or header, before declaring verified installation. This
defers verification until after boot. If boot fails, the checksum is unavailable
and verification remains incomplete; a tested independent readback/recovery
path has not been established by this audit. Do not weaken the Linux storage
guard, replace loaders, change GPT/vbmeta, or broaden writes to conceal that
limitation. Existing boot-only authority does not make any such action implicit.
