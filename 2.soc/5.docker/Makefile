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
	@echo "  make clean     # Remove the Docker image"
