FROM ubuntu:22.04


# Install dependencies like wget
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    gnupg \
    wget \
    && rm -rf /var/lib/apt/lists/*

# # Add llvm apt src
# RUN cat > /etc/apt/sources.list.d/llvm.sources <<EOF
# Types: deb
# URIs: https://example.com/apt
# Suites: stable
# Components: main
# Signed-By:
# $(wget -O- https://apt.llvm.org/llvm-snapshot.gpg.key | sed -e 's/^$/./' -e 's/^/ /')
# EOF

# RUN chmod 644 /etc/apt/sources.list.d/llvm.sources

# RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
#     && echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy main" >> /etc/apt/sources.list.d/llvm.list \
#     echo deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy main\
#      >> /etc/apt/sources.list.d/llvm.list
# echo deb-src http://apt.llvm.org/jammy/ llvm-toolchain-jammy main\
#      >> /etc/apt/sources.list.d/llvm.list
#     && apt-get update

# RUN apt-get update && apt-get install -y --no-install-recommends \
#     clang-17 \
#     llvm-17 \
#     && rm -rf /var/lib/apt/lists/*


RUN \
    echo deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-17 main >> /etc/apt/sources.list.d/llvm.list && \
    echo deb-src http://apt.llvm.org/jammy/ llvm-toolchain-jammy-17 main >> /etc/apt/sources.list.d/llvm.list && \
    wget -q -O - http://apt.llvm.org/llvm-snapshot.gpg.key|apt-key add - && \
    apt-get update && apt-get install -y clang-17 llvm-17
