#!/usr/bin/env bash
# Packages the native release files of one system: zc, its standalone tools and the standard
# library, in out/dist/zenc-<version>-<system>.zip. Before zipping, it checks that they run
# from that directory, away from the repository, as someone who unpacks the zip runs them.
#
# Usage: scripts/package_release.sh VERSION SYSTEM    (e.g. 0.5.1 linux-x86_64)
# Run it from the repository, after building zc and the tools (make, or build.bat on Windows).
set -euo pipefail

version=$1
system=$2
name="zenc-$version-$system"
out="out/dist"
stage="$out/$name"

exe=""
case "$system" in
windows-*) exe=".exe" ;;
esac

rm -rf "$stage" "$out/$name.zip"
mkdir -p "$stage"
for program in zc zc-lsp zc-repl zc-doc zc-format; do
    cp "$program$exe" "$stage/"
done
cp -R std.zc std LICENSE "$stage/"
cp src/repl/docs.json src/misc/zenc.json "$stage/"
stage_dir=$(cd "$stage" && pwd)

# zc finds the standard library next to itself, and compiles with the system's C compiler
check=$(mktemp -d)
printf 'import "std/io.zc"\nfn main() { println(f"release-ok"); }\n' > "$check/hello.zc"
"$stage_dir/zc$exe" --version
# The output goes to a variable first: grep -q stops reading at the match, and with pipefail
# the program it leaves writing would fail the pipeline
ran=$(cd "$check" && "$stage_dir/zc$exe" run hello.zc 2>&1) || { echo "$ran"; exit 1; }
if ! grep -q release-ok <<< "$ran"; then
    echo "$ran"
    echo "zc could not build and run a program that imports the standard library"
    exit 1
fi
# zc-lsp answers over standard input and output
body='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"rootPath":".","capabilities":{}}}'
answer=$(cd "$check" && printf 'Content-Length: %d\r\n\r\n%s' "${#body}" "$body" | "$stage_dir/zc-lsp$exe")
if ! grep -q '"capabilities"' <<< "$answer"; then
    echo "zc-lsp did not answer initialize"
    exit 1
fi
rm -rf "$check"

cd "$out"
if command -v zip > /dev/null; then
    zip -qr "$name.zip" "$name"
else
    # Windows runners have 7-Zip, not zip
    7z a -tzip -bso0 "$name.zip" "$name"
fi
echo "$out/$name.zip"
