# =============================================================================
# Reproducible environment for Astra-Sim Garnet AE (no source changes required)
# Base: Ubuntu 20.04 (glibc 2.31)
# Tooling via Miniconda: python=2.7, protobuf=3.6.1, scons=3.1.2, etc.
# =============================================================================
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Etc/UTC

# System deps (avoid installing scons via apt to prevent conflicts with conda)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential git wget curl ca-certificates pkg-config \
    cmake make m4 \
    zlib1g-dev \
    graphviz pango1.0-tools \
    libpng-dev \
    libtcmalloc-minimal4 libgoogle-perftools-dev \
    tcsh \
 && rm -rf /var/lib/apt/lists/*

# Install Miniconda
ENV CONDA_DIR=/opt/conda
RUN curl -fsSL https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -o /tmp/miniconda.sh \
 && bash /tmp/miniconda.sh -b -p $CONDA_DIR \
 && rm -f /tmp/miniconda.sh
ENV PATH=$CONDA_DIR/bin:$PATH

WORKDIR /workspace
COPY . /workspace

# Use bash for subsequent RUN steps
SHELL ["/bin/bash", "-lc"]

# Accept Anaconda ToS and configure conda
RUN conda --version \
 && conda config --system --set always_yes true \
 && conda config --system --set channel_priority flexible \
 && conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/main \
 && conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/r \
 && conda init bash

# Create the conda env defined by AE (env name expected to be 'astra-sim-garnet')
RUN conda env create -f garnet_backend/astra-sim-garnet.yml \
 && echo "conda activate astra-sim-garnet" >> /root/.bashrc

# Sanity check (optional)
RUN source $CONDA_DIR/etc/profile.d/conda.sh \
 && conda activate astra-sim-garnet \
 && python --version \
 && scons --version || true

# Do NOT compile at build time (we compile with a bind mount so artifacts stay on host)
CMD ["/bin/bash", "-lc", "source /opt/conda/etc/profile.d/conda.sh && conda activate astra-sim-garnet && bash"]

# --- System-wide fallbacks so tcsh/SCons can find things without env vars ---

# 1) Ensure pkg-config always finds protobuf, even if PKG_CONFIG_PATH is lost.
if [ -w /usr/lib/pkgconfig ]; then
    mkdir -p /usr/lib/pkgconfig
    ln -sf "$HOME/pkgconfig/protobuf.pc" /usr/lib/pkgconfig/protobuf.pc
fi

# 2) Ensure the linker can always find libpython2.7 without extra -L flags.
if [ -f "$CONDA_PREFIX/lib/libpython2.7.so" ] && [ -w /usr/lib ]; then
    ln -sf "$CONDA_PREFIX/lib/libpython2.7.so" /usr/lib/libpython2.7.so
fi