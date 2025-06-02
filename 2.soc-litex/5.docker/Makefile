# Makefile for building and running the FPGA Docker environment

# Image and container names
IMAGE           := litex-env
CONTAINER_NAME  := litex-container

# Host project directory (mounted into /workspace inside container)
HOST_DIR        := $(shell pwd)
CONTAINER_WORK  := /workspace

# Default target: build the Docker image
.PHONY: all
all: build

# Build the Docker image using the Dockerfile in the current directory
.PHONY: build
build:
	@echo "Building Docker image '$(IMAGE)'…"
	docker build -t $(IMAGE) .

# Run an interactive bash shell inside the container, mounting the current directory
.PHONY: shell
shell:
	@echo "Launching container '$(CONTAINER_NAME)' with an interactive shell…"
	docker run --rm -it \
		--name $(CONTAINER_NAME) \
		-v $(HOST_DIR):$(CONTAINER_WORK) \
		$(IMAGE) \
		/bin/bash

# Run LXQt GUI session (requires a local X server).
# Assumes Docker has permission to connect to $DISPLAY.
.PHONY: gui
gui:
	@echo "Granting X11 permissions and launching LXQt desktop…"
	xhost +local:docker >/dev/null 2>&1 || true
	docker run --rm -it \
		--name $(CONTAINER_NAME) \
		-e DISPLAY=$(DISPLAY) \
		-v /tmp/.X11-unix:/tmp/.X11-unix \
		-v $(HOST_DIR):$(CONTAINER_WORK) \
		$(IMAGE)

# Remove the local Docker image
.PHONY: clean
clean:
	@echo "Removing Docker image '$(IMAGE)'…"
	docker rmi $(IMAGE) || true

# Show usage
.PHONY: help
help:
	@echo "Usage:"
	@echo "  make build     # Build the Docker image"
	@echo "  make shell     # Run an interactive shell in the container"
	@echo "  make gui       # Run an LXQt desktop session (requires X11 forwarding)"
	@echo "  make clean     # Remove the Docker image"
