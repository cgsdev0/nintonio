{
  description = "Static ARMv5TE musl cross-compilation flake for Kobo N647 (i.MX35, ARM1136 core, ARMv5TE userland ABI, kernel 2.6.x)";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      mkPkgs = system: import nixpkgs {
        inherit system;
        crossSystem = {
          # Confirmed via readelf -A on /usr/local/Kobo/pickel:
          #   Tag_CPU_arch: v5TE
          #   Tag_DIV_use: Not Permitted
          #   no Tag_FP_arch / Tag_ABI_VFP_args -> no VFP codegen at all
          #
          # musleabi (not gnueabi): nixpkgs glibc is configured with
          # --enable-kernel=3.2, which hard-fails at runtime ("FATAL:
          # kernel too old") on this device's 2.6.x kernel. musl has no
          # such gate and is far more tolerant of old kernels.
          config = "armv5tel-unknown-linux-musleabi";
          gcc = {
            arch = "armv5te";
            float-abi = "soft";
            # NOTE: do not set fpu = "none" -- gcc's configure rejects it
            # ("Unknown target in --with-fpu=none"). Omitting fpu entirely
            # is correct here since armv5te has no VFP option at all.
          };
        };
      };
    in {
      packages = forAllSystems (system:
        let staticPkgs = (mkPkgs system).pkgsStatic;
        in {
          default = staticPkgs.stdenv.mkDerivation {
            pname = "hello-kobo";
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

          # trivial ABI-sanity-check binary, no libc formatting/float paths at all
          trivial = staticPkgs.stdenv.mkDerivation {
            pname = "trivial-kobo";
            version = "1.0.0";
            src = ./.;
            buildPhase = ''
              $CC -static -O2 trivial.c -o trivial
            '';
            installPhase = ''
              mkdir -p $out/bin
              cp trivial $out/bin/
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
