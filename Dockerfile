FROM ubuntu:24.04

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
        autoconf build-essential ca-certificates git gnupg \
        cmake libcgal-dev libspectra-dev libeigen3-dev clang \
        clangd gpg libfmt-dev libtool libssl-dev lsb-release \
        software-properties-common unzip nginx wget
EOF

RUN /bin/bash <<EOF
    add-apt-repository ppa:berndporr/dsp
    apt-get update
    apt-get install -y --no-install-recommends \
        iir1 \
        iir1-dev \
        libspdlog-dev
EOF

# Expose port 3000 for NGINX
EXPOSE 3000, 3333

COPY ./nginx.conf /etc/nginx/nginx.conf
RUN chown -R ubuntu:ubuntu /var/lib/nginx /var/log/nginx \
    && chmod -R 755 /var/lib/nginx /var/log/nginx \
    # See https://stackoverflow.com/a/64426330
    && touch /run/nginx.pid \
    && chown -R ubuntu:ubuntu /run/nginx.pid

USER ubuntu
# Use bash for the shell
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
# Create a script file sourced by both interactive and non-interactive bash shells
ENV BASH_ENV="/home/ubuntu/.bash_env"
RUN touch "${BASH_ENV}"
RUN echo 'source "${BASH_ENV}"' >> ~/.bashrc
# Download and install nvm
RUN wget -qO- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | PROFILE="${BASH_ENV}" bash
RUN nvm install 22.19.0

# Use heredoc to make multi-line CMD
RUN tee /tmp/cmd.sh <<EOF
cmake -B build
cmake --build build
./build/bonk
EOF

CMD ["bash"]