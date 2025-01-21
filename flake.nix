{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      tools = with pkgs; [
        gcc
        gdb
        ethtool
        pciutils
        ncurses
      ];
    in {
      devShells.default = pkgs.mkShellNoCC {
        packages = tools;
      };
    });
}
