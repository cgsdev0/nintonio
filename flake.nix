{
  description = "Static ARMv6 Musl cross-compilation flake for legacy Kobo (Kernel 2.6)";
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      mkPkgs = system: import nixpkgs {
        inherit system;
        crossSystem = {
          config = "armv6l-unknown-linux-musleabi"; # musleabi -> musl libc
          gcc.arch = "armv6j";
          gcc.fpu = "vfp";
          gcc.float-abi = "softfp";
        };
      };
    in {
      packages = forAllSystems (system:
        let staticPkgs = (mkPkgs system).pkgsStatic;
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
      devShells = forAllSystems (system: {
        default = (mkPkgs system).mkShell {
          inputsFrom = [ self.packages.${system}.default ];
        };
      });
    };
}
