# Makefile for common Pebble SDK tasks in this repo.

.PHONY: help build run-emulator

help:
	@printf "Targets:\n"
	@printf "  build         Build the watchface\n"
	@printf "  run-emulator  Install on the emulator\n"

build:
	@scripts/build.sh

run-emulator:
	@scripts/run-emulator.sh
