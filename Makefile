# Makefile for common Pebble SDK tasks in this repo.

.PHONY: help build build-app build-settings run-emulator run-settings

help:
	@printf "Targets:\n"
	@printf "  build         Build the watchface (normal)\n"
	@printf "  build-app     Same as build\n"
	@printf "  build-settings Build with settings-only UI\n"
	@printf "  run-emulator  Install on the emulator\n"
	@printf "  run-settings  Build settings-only and install\n"

build:
	@scripts/build.sh

build-app:
	@scripts/build.sh

build-settings:
	@VIPASSANA_SETTINGS_ONLY=1 scripts/build.sh

run-emulator:
	@scripts/run-emulator.sh

run-settings:
	@VIPASSANA_SETTINGS_ONLY=1 scripts/build.sh
	@scripts/run-emulator.sh
