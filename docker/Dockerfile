# LitePB Development Container
# Optimized for CI/CD and local development environments

FROM python:3.11-slim

LABEL org.opencontainers.image.title="LitePB Development Environment" \
      org.opencontainers.image.description="Development container for LitePB - Lightweight Protocol Buffer serialization library for embedded systems" \
      org.opencontainers.image.version="1.0.1" \
      org.opencontainers.image.vendor="LitePB Project" \
      org.opencontainers.image.licenses="MIT" \
      org.opencontainers.image.source="https://github.com/litepb/litepb"

ENV DEBIAN_FRONTEND=noninteractive \
    PYTHONUNBUFFERED=1 \
    PIP_NO_CACHE_DIR=1 \
    PIP_DISABLE_PIP_VERSION_CHECK=1

RUN apt-get update && apt-get install -y --no-install-recommends \
    protobuf-compiler \
    libprotobuf-dev \
    libabsl-dev \
    cmake \
    build-essential \
    pkg-config \
    clang-format \
    lcov \
    gcc \
    g++ \
    git \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

RUN python -m pip install --upgrade pip && \
    pip install --no-cache-dir \
    platformio \
    protobuf \
    jinja2

RUN groupadd -g 1000 builder && \
    useradd -m -u 1000 -g builder -s /bin/bash builder && \
    mkdir -p /workspace && \
    chown -R builder:builder /workspace

USER builder

WORKDIR /workspace

CMD ["/bin/bash"]
