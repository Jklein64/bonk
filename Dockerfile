FROM ubuntu:25.10

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
        autoconf build-essential ca-certificates cmake wget \
        clang clangd git gnupg gpg libboost-dev libcgal-dev \
        libspectra-dev libgmp-dev libmpfr-dev libeigen3-dev \
        libtool libssl-dev lsb-release software-properties-common
EOF

USER ubuntu

# Use bash for the shell
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
# Create a script file sourced by both interactive and non-interactive bash shells
ENV BASH_ENV="/home/ubuntu/.bash_env"
RUN touch "${BASH_ENV}"
RUN echo '. "${BASH_ENV}"' >> ~/.bashrc
# Download and install nvm
RUN wget -qO- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | PROFILE="${BASH_ENV}" bash
RUN nvm install 22.19.0

# Expose port 3000 for vite dev server
EXPOSE 3000

CMD [ "bash" ]