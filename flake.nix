{
  description = "Static ARM32 cross-compilation flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      # Replace with your host system if different (e.g., "aarch64-linux")
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
    in {
      packages = forAllSystems (system:
        let
          # Import nixpkgs for your host system, targeting static armv6l cross-compilation
          pkgs = import nixpkgs {
            inherit system;
            crossSystem = {
              config = "armv6l-unknown-linux-gnueabihf"; # armv6 little endian hard-float
            };
          };

          # Use the static package set from the cross-compiler
          staticPkgs = pkgs.pkgsStatic;
        in {
          default = staticPkgs.stdenv.mkDerivation {
            pname = "hello-arm32-static";
            version = "1.0.0";
            src = ./.;

            # Explicitly force static flags during compilation
            buildPhase = ''
              $CC -static -O2 main.c -o hello
            '';

            installPhase = ''
              mkdir -p $out/bin
              cp hello $out/bin/
            '';
          };
        });

      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            crossSystem = { config = "armv6l-unknown-linux-gnueabihf"; };
          };
        in {
          default = pkgs.pkgsStatic.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
          };
        });
    };
}

