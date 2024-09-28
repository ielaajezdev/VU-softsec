FROM --platform=linux/amd64 ubuntu:24.04 

ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt upgrade -y

# Install common tools to compile 
RUN apt update --fix-missing && apt install -y \
    build-essential \
    clangd \
    git \ 
    python3 \ 
    gdb \
    python3-pip \
    python3-dev git \
    libssl-dev \
    libffi-dev \
    curl \
    file


# Install GEF
RUN bash -c "$(curl -fsSL https://gef.blah.cat/sh)"

# Install pwntools
RUN apt install -y python3-pip
RUN apt install -y python3-pwntools

# Switch to lower-privilege user
# RUN useradd -ms /bin/bash developer
# USER developer
