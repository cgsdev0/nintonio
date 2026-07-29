{
  description = "Static ARMv6 Musl cross-compilation flake for legacy Kobo (Kernel 2.6)";

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
              # 1. Must use soft-float gnueabi to prevent ABI mismatch crashes
              config = "armv6l-unknown-linux-gnueabi";
              
              # 2. Match your 6TEJ processor exactly 
              gcc.arch = "armv6"; 
              gcc.fpu = "vfp"; 
              gcc.float = "soft"; 

              platform = nixpkgs.lib.systems.platforms.raspberrypi;
            };
          };

          # 3. Swap Glibc out for Musl (pkgsStatic), which tolerates Linux 2.6 kernels
          staticPkgs = pkgs.pkgsStatic;
        in {
          default = staticPkgs.stdenv.mkDerivation {
            pname = "hello-kobo-legacy-static";
            version = "1.0.0";
            src = ./.;

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
          default = pkgs.pkgsStatic.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
          };
        });
    };
}

