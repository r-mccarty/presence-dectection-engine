FROM python:3.11-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        git \
        sudo \
        curl \
        ca-certificates \
        bash-completion \
        locales \
    && rm -rf /var/lib/apt/lists/*

# Ensure a vscode user exists for Codespaces and devcontainers
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} \
    && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}

# Allow the non-root user to manage global Python packages commonly used in dev containers.
RUN chown -R ${USERNAME}:${USERNAME} /usr/local

# Set locale data commonly expected by tools
RUN sed -i "s/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/" /etc/locale.gen \
    && locale-gen

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

# Allow pip installs to use the user base bin directory.
ENV PATH="/home/${USERNAME}/.local/bin:${PATH}"

# Install claude CLI
RUN set -eux; \
    curl -fsSL https://claude.ai/install.sh | bash || true; \
    if [ -x /root/.local/bin/claude ]; then \
      install -m 0755 /root/.local/bin/claude /usr/local/bin/claude; \
    fi

# Keep root as the default build user so features can run elevated.
# The devcontainer.json will switch to the vscode user at runtime.
