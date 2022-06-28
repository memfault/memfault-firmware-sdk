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
    git \
    python3.10 \
    python3.10-venv\
"

# Run commands and tests as circleci user
RUN echo 'APT::Get::Assume-Yes "true";' > /etc/apt/apt.conf.d/90circleci && \
    echo 'DPkg::Options "--force-confnew";' >> /etc/apt/apt.conf.d/90circleci && \
    apt-get update && apt-get install -y \
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

ENV PATH=/home/circleci/bin:/home/circleci/.local/bin:$PATH \
    LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

USER circleci

# Match the default CircleCI working directory
WORKDIR /home/circleci/project

# Create the virtualenv
RUN python3 -m venv ~/venv

# Install lcov and add to PATH
ARG LCOV_VERSION=1.14
ARG LCOV_SHA256SUM=14995699187440e0ae4da57fe3a64adc0a3c5cf14feab971f8db38fb7d8f071a
RUN \
    cd ~ && \
    wget https://github.com/linux-test-project/lcov/releases/download/v${LCOV_VERSION}/lcov-${LCOV_VERSION}.tar.gz -O ~/lcov.tar.gz && \
    echo "${LCOV_SHA256SUM}  ${HOME}/lcov.tar.gz" | shasum --algorithm=256 --check && \
    cd ~ && tar zvxf ~/lcov.tar.gz && \
    rm ~/lcov.tar.gz

ENV PATH=/home/circleci/lcov-${LCOV_VERSION}/bin:$PATH

# Auto-activate the virtualenv in the container
RUN sudo mkdir -p /circleci/ && \
    sudo bash -c "echo 'source ~/venv/bin/activate' >> /circleci/.bashrc_circleci" && \
    echo 'source /circleci/.bashrc_circleci' >> ~/.bashrc

ENTRYPOINT ["bash"]
