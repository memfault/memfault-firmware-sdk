# This Docker image is used in CircleCI to build the SDK. It's published to
# Docker Hub as "memfault/memfault-sdk-embedded-ci".
#
# It's built with the following command (assuming cwd is the directory
# containing this Dockerfile):
#
# ❯ DOCKER_BUILDKIT=1 docker build -t memfault/memfault-firmware-sdk-ci:<tag> .
#
# And uploaded to Docker Hub:
# ❯ docker push memfault/memfault-firmware-sdk-ci:<tag>

FROM ubuntu:22.04

# Some details based on this Dockerfile:
# https://github.com/CircleCI-Public/cimg-base/blob/2b2cc9584c5ce2256d0781106218ff4158c790a0/22.04/Dockerfile

SHELL ["/bin/bash", "-exo", "pipefail", "-c"]

ENV DEBIAN_FRONTEND=noninteractive \
    TERM=dumb \
    PAGER=cat


ARG MEMFAULT_SDK_APT_DEPS="\
    build-essential \
    cpputest \
    gcc-12 \
    g++-12 \
    git \
    python3.10 \
    python3.10-venv\
"

# Run commands and tests as circleci user
RUN echo 'APT::Get::Assume-Yes "true";' > /etc/apt/apt.conf.d/90circleci && \
    echo 'DPkg::Options "--force-confnew";' >> /etc/apt/apt.conf.d/90circleci && \
    apt-get update && apt-get install -y --no-install-recommends \
        locales \
        sudo \
        wget \
        ${MEMFAULT_SDK_APT_DEPS} \
    && \
    locale-gen en_US.UTF-8 && \
    rm -rf /var/lib/apt/lists/* && \
    # create the circlci user
    useradd --uid=3434 --user-group --create-home circleci && \
    echo 'circleci ALL=NOPASSWD: ALL' >> /etc/sudoers.d/50-circleci && \
    echo 'Defaults    env_keep += "DEBIAN_FRONTEND"' >> /etc/sudoers.d/env_keep && \
    sudo -u circleci mkdir /home/circleci/project && \
    sudo -u circleci mkdir /home/circleci/bin

# Select gcc-12 as the default gcc/g++ version
RUN sudo update-alternatives --install \
    /usr/bin/gcc gcc /usr/bin/gcc-12 60 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-12 \
    --slave /usr/bin/gcov gcov /usr/bin/gcov-12

ENV PATH=/home/circleci/bin:/home/circleci/.local/bin:$PATH \
    LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

# Install lcov and add to PATH
ARG LCOV_VERSION=1.16
ARG LCOV_SHA256SUM=987031ad5528c8a746d4b52b380bc1bffe412de1f2b9c2ba5224995668e3240b
RUN \
    cd /tmp && \
    wget https://github.com/linux-test-project/lcov/releases/download/v${LCOV_VERSION}/lcov-${LCOV_VERSION}.tar.gz -O lcov.tar.gz && \
    echo "${LCOV_SHA256SUM}  lcov.tar.gz" | shasum --algorithm=256 --check && \
    tar zvxf lcov.tar.gz && \
    cd lcov-${LCOV_VERSION} && sudo make install && \
    cd .. && \
    rm -rf ./lcov* && \
    lcov --version && \
    genhtml --version

ENV PATH=/home/circleci/lcov-${LCOV_VERSION}/bin:$PATH

USER circleci

# Match the default CircleCI working directory
WORKDIR /home/circleci/project

# Create the virtualenv
RUN python3 -m venv ~/venv

# Auto-activate the virtualenv in the container
RUN sudo mkdir -p /circleci/ && \
    sudo bash -c "echo 'source ~/venv/bin/activate' >> /circleci/.bashrc_circleci" && \
    echo 'source /circleci/.bashrc_circleci' >> ~/.bashrc

ENTRYPOINT ["bash"]
