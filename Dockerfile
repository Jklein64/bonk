FROM ubuntu:20.04

# Never prompts the user for choices on installation/configuration of packages
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/New_York
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en

# One big install command
RUN /bin/bash <<EOF
    set -euxo pipefail

    apt-get update
    apt-get install -y --no-install-recommends \
        autoconf \
        build-essential \
        ca-certificates \
        git \
        gnupg \
        gpg \
        libeigen3-dev \
        libfmt-dev \
        libtool \
        libssl-dev \
        lsb-release \
        software-properties-common \
        unzip \
        wget
    # Install CMake. See https://askubuntu.com/a/865294
    apt-get remove --purge --auto-remove cmake
    test -f /usr/share/doc/kitware-archive-keyring/copyright || wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null
    apt-get update
    test -f /usr/share/doc/kitware-archive-keyring/copyright || rm /usr/share/keyrings/kitware-archive-keyring.gpg
    apt-get install -y --no-install-recommends kitware-archive-keyring
    apt-get update
    apt-get install -y --no-install-recommends cmake
    # Install Clang (mainly for clangd). See https://askubuntu.com/a/1508280
    wget -qO- https://apt.llvm.org/llvm.sh | bash -s -- 18
    # Install iir for filtering
    add-apt-repository ppa:berndporr/dsp && apt install iir1 iir1-dev
EOF


# Create a non-root user
RUN useradd -ms /bin/bash bonk-dev
USER bonk-dev

# Use bash for the shell
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
# Create a script file sourced by both interactive and non-interactive bash shells
ENV BASH_ENV="/home/bonk-dev/.bash_env"
RUN touch "${BASH_ENV}"
RUN echo '. "${BASH_ENV}"' >> ~/.bashrc
# Download and install nvm
RUN wget -qO- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | PROFILE="${BASH_ENV}" bash
RUN nvm install 22.19.0

# Expose port 3000 for vite dev server
EXPOSE 3000

# Use heredoc to make multi-line CMD
RUN tee /tmp/cmd.sh <<EOF
    cmake -B build
    cmake --build build
    ./build/bonk
EOF

CMD ["/bin/bash", "/tmp/cmd.sh"]