{
  description = "Pebble Vipassana dev environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            clang
            curl
            git
            gnumake
            libpng
            pkg-config
            python3
            python3Packages.pip
            python3Packages.virtualenv
            zlib
          ];

          shellHook = ''
            export PEBBLE_PROJECT_ROOT="$PWD"
            export PEBBLE_SDK_HOME="$PWD/.pebble-sdk"
            export PEBBLE_TOOL_PATH="$PEBBLE_SDK_HOME/bin"
            export PATH="$PEBBLE_TOOL_PATH:$PATH"
            export PIP_DISABLE_PIP_VERSION_CHECK=1
          '';
        };
      }
    );
}
