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
            nodejs
            pkg-config
            python313
            uv
            zlib
          ];

          shellHook = ''
            export PEBBLE_PROJECT_ROOT="$PWD"
            export PEBBLE_SDK_HOME="$PWD/.pebble-sdk"
            export PEBBLE_TOOL_PATH="$PEBBLE_SDK_HOME/bin"
            if [ -n "$XDG_BIN_HOME" ]; then
              export PATH="$XDG_BIN_HOME:$PEBBLE_TOOL_PATH:$PATH"
            else
              export PATH="$HOME/.local/bin:$PEBBLE_TOOL_PATH:$PATH"
            fi
            export PIP_DISABLE_PIP_VERSION_CHECK=1
          '';
        };
      }
    );
}
