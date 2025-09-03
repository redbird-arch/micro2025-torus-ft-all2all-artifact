#!/usr/bin/env bash
set -euo pipefail

# --- Preconditions ---
if [[ -z "${CONDA_PREFIX:-}" ]]; then
  echo "Please activate your Conda environment first (e.g., conda activate astra-sim-garnet)."
  exit 1
fi

PROTOBUF_VERSION="3.6.1"
PROTOBUF_DIR="$CONDA_PREFIX"

echo "Current Conda environment path: $PROTOBUF_DIR"
echo "Preparing to generate pkg-config file for protobuf ${PROTOBUF_VERSION}..."

# --- Create a user pkgconfig and write protobuf.pc (fallback .pc we control) ---
mkdir -p "$HOME/pkgconfig"
cat > "$HOME/pkgconfig/protobuf.pc" <<EOF
prefix=$PROTOBUF_DIR
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: protobuf
Description: Protocol Buffers
Version: ${PROTOBUF_VERSION}
Libs: -L\${libdir} -lprotobuf
Cflags: -I\${includedir}
EOF
echo "Generated: $HOME/pkgconfig/protobuf.pc"

# --- Session-scoped vars (helpful in interactive shells) ---
export PKG_CONFIG_PATH="$CONDA_PREFIX/lib/pkgconfig:$HOME/pkgconfig:${PKG_CONFIG_PATH:-}"
echo "PKG_CONFIG_PATH updated: $PKG_CONFIG_PATH"

export LD_LIBRARY_PATH="$CONDA_PREFIX/lib:${LD_LIBRARY_PATH:-}"
echo "LD_LIBRARY_PATH updated: $LD_LIBRARY_PATH"

# Also help compilers directly
export CPPFLAGS="-I$CONDA_PREFIX/include ${CPPFLAGS:-}"
export LDFLAGS="-L$CONDA_PREFIX/lib ${LDFLAGS:-}"
export LIBRARY_PATH="$CONDA_PREFIX/lib:${LIBRARY_PATH:-}"

echo "Testing pkg-config output:"
pkg-config --cflags --libs protobuf || echo "pkg-config still cannot find protobuf, please check the paths."
echo "CPPFLAGS=$CPPFLAGS"
echo "LDFLAGS=$LDFLAGS"
echo "LIBRARY_PATH=$LIBRARY_PATH"
pkg-config --modversion protobuf || echo "no protobuf via pkg-config"
ls -l "$CONDA_PREFIX/lib/libpython2.7"* || echo "libpython2.7.* not found"

# --- System-wide fallbacks so tcsh/SCons (no env) still find protobuf & python2.7 ---

# Detect the multiarch libdir used by Ubuntu 20.04 (e.g., x86_64-linux-gnu)
MULTIARCH="$(gcc -print-multiarch 2>/dev/null || true)"
if [[ -z "$MULTIARCH" ]]; then
  # Fallback; rarely needed
  MULTIARCH="x86_64-linux-gnu"
fi

SYS_PKGCFG_DIRS=(
  "/usr/lib/${MULTIARCH}/pkgconfig"
  "/usr/share/pkgconfig"
)
SYS_LIB_DIRS=(
  "/usr/lib/${MULTIARCH}"
  "/usr/lib"
)

# 1) Make protobuf.pc visible on default pkg-config search path
for d in "${SYS_PKGCFG_DIRS[@]}"; do
  if [[ -d "$d" && -w "$d" ]]; then
    ln -sf "$HOME/pkgconfig/protobuf.pc" "$d/protobuf.pc"
    echo "Linked protobuf.pc into: $d"
  fi
done

# 2) Make libpython2.7 visible to the default linker search path
if [[ -f "$CONDA_PREFIX/lib/libpython2.7.so" ]]; then
  for d in "${SYS_LIB_DIRS[@]}"; do
    if [[ -d "$d" && -w "$d" ]]; then
      ln -sf "$CONDA_PREFIX/lib/libpython2.7.so" "$d/libpython2.7.so"
      echo "Linked libpython2.7.so into: $d"
    fi
  done
fi

# Optional: refresh ld cache if available (non-fatal)
if command -v ldconfig >/dev/null 2>&1; then
  ldconfig || true
fi

# --- Verify that defaults now work WITHOUT PKG_CONFIG_PATH & extra -L ---
echo "Verifying default search paths:"
echo "pkg-config pc_path: $(pkg-config --variable=pc_path pkg-config || true)"
( env -u PKG_CONFIG_PATH pkg-config --modversion protobuf && echo "OK: default pkg-config can find protobuf" ) || echo "WARN: default pkg-config still cannot find protobuf"
( echo "int main(){return 0;}" > /tmp/try.c && g++ /tmp/try.c -lpython2.7 -o /tmp/t && rm -f /tmp/try.c /tmp/t && echo "OK: default linker can find -lpython2.7" ) || echo "WARN: default linker cannot find -lpython2.7"
