FROM archlinux:latest


RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm bash sudo git github-cli which less openssh vim && \
    # Clean up pacman cache
    pacman -Scc --noconfirm

RUN useradd -m -G wheel -s /bin/bash codespace && \
    # Setup passwordless sudo for the codespace user
    echo 'codespace ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/codespace && \
    chmod 0440 /etc/sudoers.d/codespace