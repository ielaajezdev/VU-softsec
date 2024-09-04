FROM --platform=linux/amd64 ubuntu:24.04 

ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt upgrade -y

# Install common tools to compile 
RUN apt update --fix-missing && apt install -y \
    build-essential \
    clangd \
    git \ 
    python3

# Switch to lower-privilege user
# RUN useradd -ms /bin/bash developer
# USER developer
