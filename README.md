# Alif-DK-E8-CSP4CMSIS

Example and test applications demonstrating [CSP4CMSIS](https://github.com/OliverFaust/CSP4CMSIS)
on the Alif Ensemble DevKit-E8 (Cortex-M55).

Sibling to [HimaxWE2-CSP4CMSIS](https://github.com/OliverFaust/HimaxWE2-CSP4CMSIS)
— same library, different board. CSP4CMSIS itself is board- and
RTOS-backend-agnostic; this repo is where it's exercised on real Alif
DK-E8 hardware.

## Contents

- **`neuropathway/`** — the CSP book's "Neuropathway" vision pipeline
  (Camera → Inference → Console, as a CSP process network) ported to the
  DK-E8.
- **`csp4cmsis_alt_test/`** — a minimal ALT/select smoke test; the
  reference test used throughout CSP4CMSIS's own RTOS2/RTX5 portability
  validation.
- **`csp4cmsis_pack_test/`** — proves CSP4CMSIS is genuinely installable
  and usable as a CMSIS-Pack (`OliverFaust::CSP4CMSIS`), not just as raw
  source.

All three consume CSP4CMSIS as a packaged component
(`OliverFaust::CSP4CMSIS:Core`) rather than embedding the library source.

## Prerequisites (Ubuntu, x86_64)

Tested on Ubuntu 22.04/24.04. Run as a regular user; only the package
manager step needs `sudo`.

### 1. Base build tools

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build python3 python3-pip \
    git curl unzip xz-utils libncurses5 libncurses-dev
```

### 2. Arm GNU Toolchain (arm-none-eabi-gcc)

```bash
cd ~
curl -L -o arm-gnu-toolchain.tar.xz \
  "https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz"
mkdir -p ~/tools
tar -xf arm-gnu-toolchain.tar.xz -C ~/tools
echo 'export PATH="$HOME/tools/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
arm-none-eabi-gcc --version
```
> Check [Arm's GNU toolchain downloads page](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
> for the current release if `13.3.rel1` is no longer the latest — the
> filename in the URL will need updating to match.

**This alone is not enough for CMSIS-Toolbox.** Putting the compiler on
`PATH` lets you run it directly, but `csolution`/`cbuild` deliberately
don't rely on `PATH` to find toolchains — picking up whatever `gcc`
happens to resolve to is explicitly *not* how Arm's own tooling wants this
done. Instead it requires **explicit toolchain registration** via an
environment variable named `GCC_TOOLCHAIN_<major>_<minor>_<patch>`,
pointing at the compiler's `bin/` directory. Without this, `csolution`/
`cbuild` report `warning csolution: No compiler registered` and the build
won't proceed.

```bash
arm-none-eabi-gcc --version
# e.g. "arm-none-eabi-gcc (Arm GNU Toolchain 13.3.rel1) 13.3.1 20240614" -> use 13_3_1

echo 'export GCC_TOOLCHAIN_13_3_1="$HOME/tools/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi/bin"' >> ~/.bashrc
source ~/.bashrc
```
> The version numbers in the variable name come from the *underlying GCC
> version* reported by `arm-none-eabi-gcc --version` (e.g. `13.3.1`), not
> from Arm's release label (`13.3.rel1`) — check your actual `--version`
> output rather than assuming these match, and update the variable name
> if you installed a different version.

### 3. CMSIS-Toolbox (`csolution`, `cbuild`, `cpackget`)

```bash
cd ~/tools
curl -L -o cmsis-toolbox.tar.gz \
  "https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases/latest/download/cmsis-toolbox-linux-amd64.tar.gz"
tar -xzf cmsis-toolbox.tar.gz
echo 'export PATH="$HOME/tools/cmsis-toolbox-linux-amd64/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
cbuild --version
cpackget --version
```

### 4. Initialize the CMSIS-Pack root

```bash
export CMSIS_PACK_ROOT="$HOME/.cache/arm/packs"
echo 'export CMSIS_PACK_ROOT="$HOME/.cache/arm/packs"' >> ~/.bashrc
cpackget init https://www.keil.com/pack/index.pidx
```

### 5. Register the CSP4CMSIS pack

`OliverFaust::CSP4CMSIS` is **not** in Arm's public pack index — it's
self-hosted via GitHub Releases (see
[CSP4CMSIS's own README](https://github.com/OliverFaust/CSP4CMSIS#getting-started)
for why). `cpackget init` above only knows about the public index, so this
pack has to be added explicitly, once, before any of the examples in this
repo can build:

```bash
cpackget add https://github.com/OliverFaust/CSP4CMSIS/releases/download/v1.0.0/OliverFaust.CSP4CMSIS.1.0.0.pack
```

> Confirmed working: fetch the `.pack` archive directly (not the bare
> `.pdsc`) from the concrete, versioned release URL — not
> `releases/latest/download/...`. `cpackget add` handles a `.pdsc`-only
> URL as a local-file reference (for active pack development), not a
> remote fetch; the `.pack` archive is the installable unit and is what
> actually downloads over HTTPS. Add `-F`/`--force-reinstall` if you need
> to reinstall an already-installed version. Update to a newer CSP4CMSIS
> release deliberately by changing the version in the URL, or via
> `cpackget update OliverFaust::CSP4CMSIS`.

Confirm it registered correctly:
```bash
cpackget list
# should include: OliverFaust::CSP4CMSIS@<version>
```

If you ever update CSP4CMSIS to a newer release, re-run the `cpackget add`
above (or `cpackget update OliverFaust::CSP4CMSIS`) to pick it up — it
won't update automatically.

### 6. J-Link tools (for flashing)

Download the **J-Link Software and Documentation Pack** for Linux
(x86_64, `.deb`) from
[segger.com/downloads/jlink](https://www.segger.com/downloads/jlink/)
(requires accepting SEGGER's license on their site — can't be scripted
via a stable direct URL) and install:
```bash
sudo dpkg -i JLink_Linux_*_x86_64.deb
```

### 7. Alif SETOOLS

Alif's `app-release-exec-linux` packaging tool and Secure Enclave
services are required to produce a flashable image from the build output.
Obtain these from Alif's own SDK/DevKit resources (Alif's developer
portal, under the Ensemble/DK-E8 SDK download) — not redistributed here.
Once obtained, note the install path; it's referenced in the flashing
section below.

## Clone

```bash
git clone https://github.com/OliverFaust/Alif-DK-E8-CSP4CMSIS.git
cd Alif-DK-E8-CSP4CMSIS
```

## Build

Each example is its own CMSIS-Toolbox solution. From inside the example's
directory:

```bash
cd csp4cmsis_alt_test
cbuild CSP4CMSIS_AltTest.csolution.yml --packs
```
(`--packs` fetches any *other* missing dependency packs referenced by the
solution — CMSIS-CORE, the RTOS2 backend, the DK-E8 BSP, etc. It does
**not** register `OliverFaust::CSP4CMSIS` itself if you skipped step 5
above; that one has to be added explicitly first, once, since it isn't in
the public index `--packs` searches.)

Substitute `neuropathway/Neuropathway.csolution.yml` or
`csp4cmsis_pack_test/`'s own `.csolution.yml` for the other examples. Add
`-c .+Debug` or `-c .+Release` to build a specific configuration; omitting
`-c` builds every context the solution defines.

The build output (`.axf`/`.bin`/`.hex`/`.map`) lands under that project's
`out/` directory.

## Flash

Flashing goes through Alif's SETOOLS (`tools/setools/app-release-exec-linux/`).
**All commands below must be run from inside that directory** — its
binaries resolve `utils/`/`build/` relative to the current working
directory, not relative to the executable, so running them from elsewhere
fails immediately with a `FileNotFoundError`.

Worked example below is for `csp4cmsis_alt_test` specifically — substitute
the project name, config JSON, and staged binary name for the other
examples (each has its own `app-cfg-<project>.json` under
`tools/setools/app-release-exec-linux/build/config/`).

### 1. Build

```bash
cd csp4cmsis_alt_test
cbuild CSP4CMSIS_AltTest.csolution.yml -c M55_HP.Debug+DevKit-E8
```
Output lands at
`csp4cmsis_alt_test/out/M55_HP/DevKit-E8/Debug/M55_HP.bin`.

### 2. Stage the binary and generate the APP TOC package

```bash
cp csp4cmsis_alt_test/out/M55_HP/DevKit-E8/Debug/M55_HP.bin \
   tools/setools/app-release-exec-linux/build/images/csp4cmsis_alt_test_hp.bin

cd tools/setools/app-release-exec-linux
./app-gen-toc -f build/config/app-cfg-alt-test.json -o build/AppTocPackage.bin
```
`app-cfg-alt-test.json` already carries `csp4cmsis_alt_test`'s target
MRAM address and CPU id — don't reuse another project's config against
this binary, or vice versa.

### 3. Burn it to the board over serial

```bash
./app-write-mram -a
```
- `-a` authenticates the image by sending its signature file.
- Add `-c /dev/ttyACM0` (or the correct device) if auto-detection picks
  the wrong port — `./app-write-mram -d` lists candidate COM ports.
- Add `-f` first if you need a full OSPI erase before writing (e.g.
  switching between different images).

`app-write-mram` prints its burn plan (`Burning: build/images/... 0x80200000
...`) even before a device answers — double-check the plan matches the
image you actually intend to flash before it confirms the board is
present.

### 4. Verify it booted

```bash
./maintenance -opt getcpustatus
```

### 5. Watch the serial console

```bash
minicom -D /dev/ttyACM0 -b 115200
```
Adjust the device path to match whatever `./app-write-mram -d` reported.
`115200` is this board/toolchain combination's standard console baud rate
— if output looks garbled, check the actual `Retarget-IO`/USART
configuration in the project's `RTE_Device.h` rather than assuming this
value is universally correct. Exit minicom with `Ctrl-A`, `X`.

> **Note on verification status:** the flag/invocation shape above is
> confirmed against the real SETOOLS binaries and config files. The full
> burn-then-boot cycle has not yet been exercised end-to-end against a
> physical DK-E8 in this pass — worth a real confirmed run (and updating
> this note once done) rather than treating this section as hardware-
> proven on the strength of the dry-run flag checks alone.

## Repository structure note

This repo's history was extracted with `git filter-repo` from a larger
development sandbox, preserving the real commit-by-commit history of each
project's CMSIS-RTOS2 migration and hardware validation — not squashed
into a single import commit.

## License

See [`LICENSE`](LICENSE) (or the license terms of the originating
sandbox repo, if this repo doesn't yet have its own — confirm before
publishing).
