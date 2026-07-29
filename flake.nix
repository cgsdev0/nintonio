{
  description = "Static ARMv6 glibc cross-compilation flake for Kobo i.MX35";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
    in {
      packages = forAllSystems (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            crossSystem = {
              config = "armv6l-unknown-linux-gnueabi";
              
              # GCC expects "armv6" here, which covers the 6TEJ instruction set
              gcc.arch = "armv6"; 
              gcc.fpu = "vfp"; 
              gcc.float = "soft"; # Force soft-float ABI layout matching gnueabi
              
              # Align internal nixpkgs layouts with the legacy target platform
              platform = nixpkgs.lib.systems.platforms.raspberrypi;
            };
          };
        in {
          default = pkgs.stdenv.mkDerivation {
            pname = "hello-kobo-armv6-static";
            version = "1.0.0";
            src = ./.;

            # Leverage the cross-compiled static glibc layer
            buildInputs = [ pkgs.glibc.static ];

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
            crossSystem = { 
              config = "armv6l-unknown-linux-gnueabi";
              gcc.arch = "armv6";
              gcc.fpu = "vfp";
              gcc.float = "soft";
              platform = nixpkgs.lib.systems.platforms.raspberrypi;
            };
          };
        in {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
          };
        });
    };
}

