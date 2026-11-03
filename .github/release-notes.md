## Downloads

Every download needs a C compiler on the system (GCC or Clang; on Windows, GCC from MinGW-w64), except `zc-boot.com`, which downloads one. [Install](https://zenc-lang-ce.github.io/docs/install/) explains each one.

| File | What it is |
|---|---|
| `zenc-<version>-linux-x86_64.tar.gz`, `zenc-<version>-linux-arm64.tar.gz` | Native binaries for Linux (glibc 2.34 or later): `zc`, `zc-lsp`, `zc-repl`, `zc-format`, `zc-doc` and the standard library. Unpack it (`tar -xzf`) and add the directory to your `PATH`. |
| `zenc-<version>-macos-arm64.zip`, `zenc-<version>-macos-x86_64.zip` | The same for macOS 11 or later, on Apple silicon and Intel. They are not signed: `xattr -dr com.apple.quarantine <directory>`. |
| `zenc-<version>-windows-x86_64.zip` | The same for Windows: `zc.exe` and the tools. |
| `zc.com` | The compiler as a portable executable: one file that runs on Linux, macOS, Windows and the BSDs, on x86_64 and arm64, with the standard library inside. On Windows, rename it to `zc.exe`. |
| `zenc-<version>-ape.zip` | `zc.com` and the tools as portable executables: `zc-lsp.com`, `zc-repl.com`, `zc-format.com` and `zc-doc.com`. Keep them in one directory. |
| `zc-boot.com` | For a machine with no C compiler: run it in an empty directory, and it downloads the Cosmopolitan C compiler and the `zc.com` of this release. |

None of them has the REPL JIT; build from source with libtcc to get it.
