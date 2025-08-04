#!/bin/bash

# ==============================
# Auto-configure Protobuf for pkg-config
# ==============================

# 1. Check if a Conda environment is activated
if [ -z "$CONDA_PREFIX" ]; then
    echo "Please activate your Conda environment first."
    exit 1
fi

PROTOBUF_VERSION=3.6.1
PROTOBUF_DIR=$CONDA_PREFIX

echo "Current Conda environment path: $PROTOBUF_DIR"
echo "Preparing to generate pkg-config file for protobuf ${PROTOBUF_VERSION}..."

# 2. Create pkgconfig directory
mkdir -p $HOME/pkgconfig

# 3. Generate protobuf.pc file
cat > $HOME/pkgconfig/protobuf.pc <<EOF
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

# 4. Update PKG_CONFIG_PATH (only for current session)
export PKG_CONFIG_PATH=$HOME/pkgconfig:$PKG_CONFIG_PATH
echo "PKG_CONFIG_PATH updated: $PKG_CONFIG_PATH"

# 5. Update LD_LIBRARY_PATH (only for current session)
export LD_LIBRARY_PATH=$CONDA_PREFIX/lib:$LD_LIBRARY_PATH
echo "LD_LIBRARY_PATH updated: $LD_LIBRARY_PATH"

# 6. Test pkg-config
echo "Testing pkg-config output:"
pkg-config --cflags --libs protobuf || echo "pkg-config still cannot find protobuf, please check the paths."