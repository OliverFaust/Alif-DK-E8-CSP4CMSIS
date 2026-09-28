# Alif-DK-E8-CSP4CMSIS

Example and test applications demonstrating [CSP4CMSIS](https://github.com/OliverFaust/CSP4CMSIS)
on the Alif Ensemble DevKit-E8 (Cortex-M55).

Sibling to [HimaxWE2-CSP4CMSIS](https://github.com/OliverFaust/HimaxWE2-CSP4CMSIS)
— same library, different board. CSP4CMSIS itself is board- and
RTOS-backend-agnostic; this repo is where it is exercised on real Alif
DK-E8 hardware.

## Contents

- **`csp4cmsis_alt_test/`** — a minimal ALT/select smoke test. **Start
  here.**
- **`csp4cmsis_pack_test/`** — proves CSP4CMSIS is installable and usable
  as a CMSIS-Pack (`OliverFaust::CSP4CMSIS`), not just as raw source.
- **`neuropathway/`** — the CSP book's "Neuropathway" pipeline, adapted
  to the IMU: an ICM-42670 accelerometer feeds 128-sample windows to an
  ExecuTorch model on the Ethos-U55 NPU, which classifies them as WALKING
  or LAYING (Sensor → Inference → Console, as a CSP process network). It
  needs an extra pack; see [Building neuropathway](#building-neuropathway).
  Build `csp4cmsis_alt_test` first.

All three consume CSP4CMSIS as a packaged component
(`OliverFaust::CSP4CMSIS:Core`) rather than embedding the library source.

## What you need

| Goal | Needs |
|---|---|
| **Part 1 — Build** | An x86_64 Ubuntu machine with internet access. No Alif tools and no board. |
| **Part 2 — Flash and run** | Everything in Part 1, plus a DK-E8, a USB cable, Alif's SETOOLS and `minicom`. |

Target platform is Ubuntu 24.04 on x86_64. Other releases are untested.

---

# Part 1 — Build (no board needed)

## 1. Base build tools

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build python3 git curl unzip xz-utils
```

Ubuntu 24.04 ships a recent CMake. On older releases, check
`cmake --version` against the minimum stated in the CMSIS-Toolbox
installation docs.

## 2. Arm GNU Toolchain 14.2.rel1

```bash
mkdir -p ~/tools
cd ~/tools
curl -L -o arm-gnu-toolchain.tar.xz \
  "https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz"
tar -xf arm-gnu-toolchain.tar.xz
echo 'export PATH="$HOME/tools/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin:$PATH"' >> ~/.bashrc
echo 'export GCC_TOOLCHAIN_14_2_1="$HOME/tools/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin"' >> ~/.bashrc
source ~/.bashrc
arm-none-eabi-gcc --version      # should report 14.2.1
```

**Both lines matter.** `PATH` lets you run the compiler yourself. The
`GCC_TOOLCHAIN_14_2_1` variable is how CMSIS-Toolbox finds it:
`csolution`/`cbuild` do not rely on `PATH`, and without the variable they
report `warning csolution: No compiler registered`. The variable name uses
the GCC version that `arm-none-eabi-gcc --version` prints (`14.2.1`), not
Arm's release label (`14.2.rel1`).

## 3. CMSIS-Toolbox 2.14.1

```bash
cd ~/tools
curl -L -o cmsis-toolbox.tar.gz \
  "https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases/download/2.14.1/cmsis-toolbox-linux-amd64.tar.gz"
tar -xzf cmsis-toolbox.tar.gz
echo 'export PATH="$HOME/tools/cmsis-toolbox-linux-amd64/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
cbuild --version
cpackget --version
```

Don't be alarmed by the numbers these print. `cbuild --version` shows
`2.14.0` and `cpackget --version` shows `2.2.1`: those are the versions of
the individual tools inside CMSIS-Toolbox 2.14.1. The toolbox release
itself is recorded in `~/tools/cmsis-toolbox-linux-amd64/manifest_2.14.1.yml`.

## 4. Initialise the pack root

```bash
echo 'export CMSIS_PACK_ROOT="$HOME/.cache/arm/packs"' >> ~/.bashrc
source ~/.bashrc
cpackget init https://www.keil.com/pack/index.pidx
```

## 5. Register the CSP4CMSIS pack

`OliverFaust::CSP4CMSIS` is **not** in Arm's public pack index; it is
self-hosted on GitHub Releases. Add it once, explicitly, before building:

```bash
cpackget add -a https://github.com/OliverFaust/CSP4CMSIS/releases/download/v1.0.0/OliverFaust.CSP4CMSIS.1.0.0.pack
cpackget list      # should include OliverFaust::CSP4CMSIS@1.0.0
```

`-a` accepts the pack's embedded MIT licence. Without it, `cpackget`
prints the licence and asks `[A]ccept [D]ecline [E]xtract`. If you don't
answer, it installs nothing and `cpackget list` shows
`(no packs installed)`. To read the licence first, run the command without
`-a` and answer `A`. The other packs downloaded in step 6 need no such
step: `--packs` accepts their licences itself.

Use the `.pack` archive at the versioned release URL. A bare `.pdsc` URL
is treated as a local file reference and fails, and the
`releases/latest/download/` alias has not been tested with `cpackget`.
Add `-F` to reinstall an already-installed version.

## 6. Clone and build

```bash
cd ~
git clone https://github.com/OliverFaust/Alif-DK-E8-CSP4CMSIS.git
cd Alif-DK-E8-CSP4CMSIS/csp4cmsis_alt_test
cbuild CSP4CMSIS_AltTest.csolution.yml --packs
```

`--packs` downloads the other packs the project needs (device pack, BSP,
CMSIS core, RTX5). The first build on a fresh machine downloads them all,
so it takes a while. It does not register `OliverFaust::CSP4CMSIS`; that
was step 5.

To build a single configuration:

```bash
cbuild CSP4CMSIS_AltTest.csolution.yml -c M55_HP.Debug+DevKit-E8      # or Release
```

Output: `out/M55_HP/DevKit-E8/<Debug|Release>/M55_HP.bin`

**Part 1 ends here.** A successful build means the toolchain, pack
registration and CSP4CMSIS component all work on your machine.

## Building neuropathway

`neuropathway` needs more than `csp4cmsis_alt_test`, so build
`csp4cmsis_alt_test` first (steps 1 to 6 above). Then fetch the ExecuTorch
pack, from the repository root:

```bash
cd ~/Alif-DK-E8-CSP4CMSIS
./scripts/fetch_executorch_pack.sh
```

and build:

```bash
cd neuropathway
cbuild Neuropathway.csolution.yml --packs -c M55_HP.Release+DevKit-E8      # or Debug
```

Output: `neuropathway/out/M55_HP/DevKit-E8/<Debug|Release>/M55_HP.bin`

What the script does:

- It fetches only the `packs/PyTorch.ExecuTorch.1.1.0-rc1-build.12/`
  directory of [ModelNova](https://github.com/Arm-Examples/ModelNova) at
  commit `1826b9883e94ed6059f8fee11f9e787eb2c64a19` into
  `tools/modelnova/`, which is where `Neuropathway.csolution.yml` looks for
  it. That is about 2 MB of download and 26 MB on disk. `tools/modelnova/`
  is ignored by git.
- The pack is BSD 3-Clause licensed (Meta Platforms, Inc. and affiliates);
  see its `LICENSE` file.
- Running it again does nothing if the pack is already there.
  `--force` deletes `tools/modelnova/` and fetches it again.

`ARM::ethos-u-core-driver` needs no extra step: `--packs` installs it from
the public pack index.

**Caveat.** This is a 1.1.0-rc1 build of the ExecuTorch pack that ModelNova
has since removed from its main branch, so the script depends on that old
commit staying available on GitHub. If it disappears, the pack has to be
hosted elsewhere, or `neuropathway` moved to a newer ExecuTorch pack from
the public index (which renames some of the components it uses).

---

# Part 2 — Flash and run (needs a DK-E8)

## Extra prerequisites

**Alif SETOOLS.** Alif's packaging and flashing tools. Get them from
Alif's DK-E8 SDK downloads (not redistributed here) and extract them into
the cloned repository:

```
Alif-DK-E8-CSP4CMSIS/tools/setools/app-release-exec-linux/
```

**minicom and serial-port permission.**

```bash
sudo apt install -y minicom
sudo usermod -aG dialout $USER      # then log out and back in
```

Without the `dialout` group, opening `/dev/ttyACM0` fails with
"permission denied".

**J-Link (optional).** None of the commands below call J-Link. Install
SEGGER's J-Link software only if you also debug with a J-Link probe.

**Board switch SW4.** SW4 selects what the board's USB serial connection
is attached to: the SE serial link that SETOOLS uses for flashing, or the
application UART that carries console output. Put it in the **SE**
position for flashing (steps 3–4) and in the **UART** position to watch
the console (step 5). Confirm the exact switch positions against the
DK-E8 user guide — this README does not record them.

## Flash `csp4cmsis_alt_test`

All SETOOLS commands must run from inside
`tools/setools/app-release-exec-linux/`. The tools look up `utils/` and
`build/` relative to the current directory and fail with a
`FileNotFoundError` from anywhere else.

**1. Build** — done in Part 1. The binary is at
`csp4cmsis_alt_test/out/M55_HP/DevKit-E8/Debug/M55_HP.bin`.

**2. Stage the image and generate the APP TOC package.** Run from the
repository root:

```bash
cd ~/Alif-DK-E8-CSP4CMSIS

mkdir -p tools/setools/app-release-exec-linux/build/images \
         tools/setools/app-release-exec-linux/build/config

cp csp4cmsis_alt_test/out/M55_HP/DevKit-E8/Debug/M55_HP.bin \
   tools/setools/app-release-exec-linux/build/images/csp4cmsis_alt_test_hp.bin

cp csp4cmsis_alt_test/app-cfg-alt-test.json \
   tools/setools/app-release-exec-linux/build/config/app-cfg-alt-test.json

cd tools/setools/app-release-exec-linux
./app-gen-toc -f build/config/app-cfg-alt-test.json -o build/AppTocPackage.bin
```

`app-cfg-alt-test.json` carries this project's target MRAM address and CPU
id. Do not reuse it for another project's binary.

**3. Burn to the board** (SW4 in the SE position):

```bash
./app-write-mram -a
```

- `-a` authenticates the image with its signature file.
- `./app-write-mram -d` lists candidate serial ports; add
  `-c /dev/ttyACM0` (or the right port) if auto-detection picks the wrong
  one.
- Add `-f` to do a full OSPI erase first, for example when switching
  between different images.
- `app-write-mram` prints its burn plan (`Burning: build/images/…`)
  before it confirms a board is present. Check that the plan names the
  image you intend to flash.

**4. Check that the core is running:**

```bash
./maintenance -opt getcpustatus
```

**5. Watch the console** (SW4 in the UART position):

```bash
minicom -D /dev/ttyACM0 -b 115200
```

Use the port that `./app-write-mram -d` reported. Exit minicom with
`Ctrl-A`, then `X`. If the output is garbled, check the baud rate against
the project's USART configuration (`RTE_Device.h`); 115200 has not been
confirmed for this project in this README.

Earlier verified runs of this test printed
`SUCCESS: 20000 messages verified` with no `DATA ERROR` lines. Occasional
garbled characters at the start of the console output have been seen
before and are a harmless UART artefact.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `warning csolution: No compiler registered` | `GCC_TOOLCHAIN_14_2_1` is unset, or its name does not match `arm-none-eabi-gcc --version`. Re-run `source ~/.bashrc`. |
| Component `OliverFaust::CSP4CMSIS` not found | Step 5 of Part 1 was skipped; run the `cpackget add` command. |
| `permission denied` on `/dev/ttyACM0` | Add yourself to `dialout`, then log out and in. |
| `FileNotFoundError` from a SETOOLS binary | Run it from inside `tools/setools/app-release-exec-linux/`. |
| No device found by `app-write-mram` | Check the USB cable and SW4, then try `./app-write-mram -d`. |

## Verification status

- **Build (Part 1).** Followed step by step in a fresh `ubuntu:24.04` container with an empty pack root. The only defect found was the missing `-a` in step 5, now fixed above. With it, `csp4cmsis_alt_test` built in Debug and Release using GCC 14.2.1 and CMSIS-Toolbox 2.14.1, with all eight CSP4CMSIS sources coming from the pack. Part 1 took about 4.5 minutes and about 225 MB of downloads. In the same kind of container, `csp4cmsis_pack_test` built in Debug and Release (`cbuild CSP4CMSIS_PackTest.csolution.yml --packs`), also with all eight CSP4CMSIS sources from the pack, and `neuropathway` built in Debug and Release following [Building neuropathway](#building-neuropathway): the script fetched about 2 MB in a few seconds, and each build took under 20 seconds.
- **Flash (Part 2).** The command shapes are confirmed against the SETOOLS binaries and config files. A full burn-and-boot run from these instructions on a clean machine has not yet been recorded. The SW4 positions and the console baud rate are unconfirmed (see above).

## License

See [`LICENSE`](LICENSE). *(TODO: confirm this repository has its own
license file before publishing.)*
