{
  description = "cross compiler environment for nix";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        nativeBuildInputs = [
          pkgs.pkgsCross.x86_64-embedded.buildPackages.gcc
          pkgs.gnumake
          pkgs.qemu
          pkgs.nasm
          pkgs.xorriso
        ];
      };
    };
}

