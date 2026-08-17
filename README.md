# App — Base Project

A minimal C++17 base project used as a teaching example for a Docker
**base / app** two-stage build. The program itself is intentionally tiny — it
initializes a logger and prints one line — so the focus stays on **how it is
structured and built**, not on what it does.

- **Language / standard**: C++17
- **Build system**: CMake 3.15+
- **Platforms**: Linux (GCC), Windows (MSVC)
- **Output**: a single executable, `build/app`

---

## 1. Project structure

```
.
├── src/                         application source
│   ├── main.cpp                 entry point — init logger, log one line, shut down
│   └── util/log/logger.h        spdlog wrapper: console + rotating file sink, LOG_* macros
│
├── 3rdparty/                    prebuilt third-party libraries (Linux + Windows binaries)
│   ├── spdlog/                  header-only logging
│   ├── libuv/                   async I/O event loop
│   ├── FFmpeg/                  video decoding (avcodec, avutil, swscale)
│   └── OpenSSL/                 cryptography (libcrypto, libssl)
│
├── CMakeLists.txt               build definition (bundled + external modes)
├── build_project_linux.sh       local build helper (clean → configure → build → run)
│
├── docker/                      Docker two-stage build
│   ├── base/Dockerfile          image 1: compiler + CMake + 3rdparty
│   ├── app/Dockerfile           image 2: source built on top of base
│   ├── app/Dockerfile.dockerignore   app-build-only ignore rules
│   └── Makefile                 make base / app / run / clean / info
├── build_docker_linux.sh        one-shot Docker build+run helper
├── .dockerignore                shared build-context ignore rules
│
├── LICENSE.txt
└── README.md
```

### Third-party libraries

All four are shipped **prebuilt** in `3rdparty/`, so nothing has to be
downloaded or compiled from source. Each folder holds both Linux (`.so`) and
Windows (`.dll`/`.lib`) binaries plus its own build script.

| Library | Purpose | Linking |
|---|---|---|
| [spdlog](3rdparty/spdlog/) | Logging | Header-only |
| [libuv](3rdparty/libuv/) | Async I/O event loop | Shared library |
| [FFmpeg](3rdparty/FFmpeg/) | Video decoding (avcodec, avutil, swscale) | Shared library |
| [OpenSSL](3rdparty/OpenSSL/) | Cryptography (libcrypto, libssl) | Shared library |

Only spdlog is actually called by the current `main.cpp`. The other three are
already wired into the include paths and link step, so you can start using them
by adding code — no build-system changes needed.

Each library exposes its headers under a namespaced folder (`spdlog/`,
`uv.h` + `uv/`, `libavcodec/` …, `openssl/`), which is why they can later be
merged into a single include directory without clashing (see the Docker section).

---

## 2. How the build is wired

### Executable

`CMakeLists.txt` builds one target named `app` from `src/main.cpp`. The build
type is fixed to `Release`, and the build date (`YYYYMMDD`) is injected as the
`APP_VERSION_DATE` compile definition.

### Where third-party libraries come from — two modes

The same `CMakeLists.txt` can find the libraries in one of two places:

| Mode | How to select | Headers / libs from |
|---|---|---|
| **Bundled** (default) | pass nothing | the repo's `3rdparty/` |
| **External** | `-DTHIRD_PARTY_ROOT=<path>` | `<path>/include` and `<path>/lib` |

- **Bundled** is what a local build uses. Behavior is unchanged from a plain
  CMake project: it reads straight out of `3rdparty/`.
- **External** is what the Docker **app** image uses. The base image installs
  every library under `/opt/third_party`, and the app build points CMake there
  with `-DTHIRD_PARTY_ROOT=/opt/third_party`.

The source code is identical in both modes — only the include/lib search paths
differ.

### Runtime library resolution (Linux)

The executable needs to find the shared `.so` files at run time. The two modes
handle this differently:

- **Bundled**: CMake sets an RPATH containing `$ORIGIN` and copies the needed
  `.so` files next to the executable, so it runs with no `LD_LIBRARY_PATH`.
- **External**: the base image registers `/opt/third_party/lib` with the dynamic
  linker (`ldconfig`), so copying is skipped — the loader finds the libraries
  system-wide.

> **Note:** with the default `--as-needed` linking, a library is only recorded
> as a runtime dependency if the program actually calls one of its symbols.
> Since `main.cpp` currently uses only header-only spdlog, `ldd build/app` shows
> just the C/C++ runtime — not libuv/FFmpeg/OpenSSL. That is expected.

---

## 3. Building locally

### Prerequisites

```bash
sudo apt install cmake build-essential
```

- CMake 3.15+
- GCC with C++17 (Linux) or Visual Studio / MSVC x64 (Windows)

### Linux — one command

```bash
./build_project_linux.sh
```

This cleans `build/`, configures, builds, and runs the program. Equivalent
manual steps:

```bash
cmake -B build -G "Unix Makefiles"
cmake --build build -j$(nproc)
./build/app
```

Expected output:

```
[HH:MM:SS] [info] [main.cpp:9] application started
```

> **Known issue (Linux): symlinks committed as text files**
> Some `.so` entries in `3rdparty` (`libuv.so`, `libuv.so.1`, `libavcodec.so`,
> `libavutil.so`, `libswscale.so`, …) may be stored as plain text files holding
> the link target name instead of real symlinks. The linker then reports
> `file format not recognized; treating as linker script`. Recreate them as real
> symlinks:
>
> ```bash
> cd 3rdparty
> for f in libuv/lib/linux/async/libuv.so libuv/lib/linux/async/libuv.so.1 \
>          FFmpeg/lib/linux/libavcodec.so FFmpeg/lib/linux/libavcodec.so.62 \
>          FFmpeg/lib/linux/libavutil.so FFmpeg/lib/linux/libavutil.so.60 \
>          FFmpeg/lib/linux/libswscale.so FFmpeg/lib/linux/libswscale.so.9; do
>   ln -sfn "$(cat "$f")" "$f"
> done
> ```

### Windows

Configure with CMake and build with MSVC (x64). The build copies the required
`.dll` files next to `app.exe` automatically. External mode is Linux-only;
on Windows always build in bundled mode (pass no `-DTHIRD_PARTY_ROOT`).

---

## 4. Building with Docker (base / app two-stage)

The Docker build splits the work by **how often things change**:

```
base image = compiler + CMake + 3rdparty libraries   → changes rarely, builds in minutes
app  image = our source code                         → changes often,  builds in seconds
```

Because the app image starts `FROM` the base image, rebuilding after a code
change recompiles only the source — it never reinstalls the compiler or the
libraries. That is the entire point of the split.

### Prerequisites

Docker installed, and the current user in the `docker` group:

```bash
docker run --rm hello-world     # prints a greeting when ready
```

If you see `permission denied ... /var/run/docker.sock`, add yourself to the
group and start a new session:

```bash
sudo usermod -aG docker $USER
newgrp docker                   # applies to the current shell; or log out and back in
```

### Option A — one-shot script (from the repo root)

```bash
./build_docker_linux.sh          # base → app → run → image sizes
./build_docker_linux.sh base     # base image only
./build_docker_linux.sh app      # app image only (base must exist)
./build_docker_linux.sh run      # run only
./build_docker_linux.sh clean    # remove both images
```

The script also checks Docker access first and prints what to verify at each
step.

### Option B — step by step (from `docker/`)

```bash
cd docker
make base      # 1) build the base image  (minutes; only when libraries change)
make app       # 2) build the app image   (seconds; every code change)
make run       # 3) run the app
make info      # 4) show image sizes
make clean     # remove both images
```

Both options always use the **repo root** as the build context (that is where
`3rdparty/` and `src/` live), regardless of the directory you run them from.

### What each stage does and what to check

**base** (`app-tutorial-base:1.0.0`)
- Installs `build-essential`, `cmake`, `make`, `pkg-config`.
- Copies only the Linux headers and `.so` files from `3rdparty/` into
  `/opt/third_party` (Windows binaries and source archives are excluded).
- Rebuilds the symlink chain `libX.so → libX.so.N → libX.so.N.M.P` and registers
  the lib dir via `ldconfig`.
- A self-check step fails the build if any header is missing or a symlink is
  broken. Look for six `[OK] libX.so -> …` lines and `all headers present`.

**app** (`app-tutorial-app:1.0.0`)
- Starts `FROM app-tutorial-base:1.0.0` — inherits the compiler, the libraries,
  and `THIRD_PARTY_ROOT`.
- Copies **only** `CMakeLists.txt` and `src/` — never `3rdparty/`, since it is
  already in the base image.
- Builds with `-DTHIRD_PARTY_ROOT=/opt/third_party` (external mode).

Confirm the app image really reuses the base libraries and carries no copy of
its own:

```bash
docker run --rm app-tutorial-app:1.0.0 ls /app
#   build  CMakeLists.txt  src         ← no 3rdparty

docker run --rm app-tutorial-app:1.0.0 readelf -d /app/build/app | grep -i path
#   RUNPATH … /opt/third_party/lib     ← wired to the base image's libraries
```

### Common issues

| Symptom | Cause / fix |
|---|---|
| `permission denied ... docker.sock` | `sudo usermod -aG docker $USER`, then `newgrp docker` or re-login |
| `pull access denied ... app-tutorial-base` | build the base image first: `make base` |
| `COPY failed: file not found` | build context not at the repo root; use `make` or the script |
| `cannot find -luv` | missing dev symlink (`libuv.so`) |
| `libuv.so.1: cannot open shared object file` | missing soname symlink, or `ldconfig` not run |
| `file format not recognized; treating as linker script` | a symlink was committed as a text file (see Known issue in §3) |

---

## 5. Logging

`Logger::init()` (in [src/util/log/logger.h](src/util/log/logger.h)) registers a
console sink plus a rotating file sink (5 MB × 3). The log file location is
platform-dependent:

| Platform | Path |
|---|---|
| Linux | `$HOME/.app/logs/app.log` |
| Windows | `%LOCALAPPDATA%\app\logs\app.log` |

If that path cannot be created, logging falls back to console only. Emit logs
with the `LOG_TRACE` / `LOG_DEBUG` / `LOG_INFO` / `LOG_WARN` / `LOG_ERROR` /
`LOG_CRITICAL` macros; each record automatically includes its call site
(`file:line`).
