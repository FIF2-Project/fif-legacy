# FIFlegacy & libFIFlegacy

**Convert legacy Fractal Image Format files (`.fif`) from the 1990s into PNG images.**

[**Documentation — fif2.org/fiflegacy**](https://fif2.org/fiflegacy/) ·
[Download](https://github.com/FIF2-Project/fif-legacy/releases/latest)

[![CI](https://github.com/FIF2-Project/fif-legacy/actions/workflows/ci.yml/badge.svg)](https://github.com/FIF2-Project/fif-legacy/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/release/FIF2-Project/fif-legacy?label=release)](https://github.com/FIF2-Project/fif-legacy/releases/latest)

---

In the 1990s, fractal images were stored in the closed file formats of Iterated
Systems, Inc. ([more about the legacy format](https://fif2.org/fif2-legacy.html)).
Today those files are hard to open. This repository contains:

- **`fiflegacy`**: a command-line tool that converts a `.fif` file to PNG with
  one command, or prints its technical details.
- **`libfiflegacy`**: the decoder as a single-file C library with one function,
  `ConvertLegacyFIFtoPNG()`.

| Component | Version |
| --- | --- |
| fiflegacy (CLI) | 1.0 |
| libfiflegacy | 1.2 |


## Download

The [latest release](https://github.com/FIF2-Project/fif-legacy/releases/latest)
contains `fiflegacy-<ver>-windows-x64.zip` with `fiflegacy.exe` for Windows 10/11
(64-bit, no installation, no other files needed) and `SHA256SUMS.txt`. On other
systems, [build from source](#building-from-source). It takes one command.

## Quick start

```sh
fiflegacy DOLPHIN.FIF                  # writes DOLPHIN.png next to it
fiflegacy DOLPHIN.FIF out/dolphin.png  # choose the output name
fiflegacy DOLPHIN.FIF out/             # output folder: keeps the base name
fiflegacy DOLPHIN.FIF --info           # technical details of the file
fiflegacy --help | --version
```

Exit codes: `0` success, `1`–`12` the library error code, `64` wrong usage.
All error codes are described in the
[documentation](https://fif2.org/fiflegacy/).

## Using the library

Add `libfiflegacy.c` to your project, either as its own source file or
`#include`d in exactly one source file:

```c
#include "libfiflegacy.c"

/* Mode 1: write a PNG file */
FIF_RESULT *r = ConvertLegacyFIFtoPNG("in/DOLPHIN.FIF", "out/dolphin.png", FIF_MODE_FILE);
if (r && r->error != FIF_OK) printf("error %d\n", r->error);
free(r);

/* Mode 2: the PNG in memory */
r = ConvertLegacyFIFtoPNG("in/DOLPHIN.FIF", NULL, FIF_MODE_MEMORY);
if (r && r->error == FIF_OK) use_png_bytes(r->data, r->size);
free(r);

/* Mode 3: information about the file, one "Key: value" per line */
r = ConvertLegacyFIFtoPNG("in/DOLPHIN.FIF", NULL, FIF_MODE_INFO);
if (r) fputs(r->text, stdout);
free(r);
```

The result is a single memory block, released with one `free(result)` in every
mode, including on errors. Other source files that only need the declarations
define `LIBFIFLEGACY_DECLARATION_ONLY` before including the file. The full
reference is in the [documentation](https://fif2.org/fiflegacy/).

## Building from source

`fiflegacy.c` includes `libfiflegacy.c`, so one file is compiled:

```sh
cc -O2 -o fiflegacy fiflegacy.c                  # Linux, macOS, BSD
clang -O2 -o fiflegacy.exe fiflegacy.c           # Windows, clang (Visual Studio)
gcc -O2 -static -s -o fiflegacy.exe fiflegacy.c  # Windows, MinGW / llvm-mingw
cl /O2 fiflegacy.c                               # Windows, MSVC
```

## Source layout

```
libfiflegacy.c   the library: decoder for FTC/FIF1 and the built-in PNG encoder
fiflegacy.c      the command-line tool
```

The history starts with `ConvertLegacyFIFtoPNG.h` 0.9, the single-header
predecessor of the library, followed by every library version as one commit.

## License

FIFlegacy and libFIFlegacy are released under the [MIT License](LICENSE).

Part of the [FIF2 project](https://github.com/FIF2-Project). The FIF2 format:
[fif2.org](https://fif2.org).
