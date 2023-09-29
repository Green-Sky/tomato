{
  description = "tomato flake";
  # bc flakes and git submodules dont really like each other, you need to
  # append '.?submodules=1' to the nix commands.

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/release-23.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs { inherit system; };
    in {
      packages.default = pkgs.stdenv.mkDerivation {
        pname = "tomato";
        version = "0.0.0";

        src = ./.;
        submodules = 1;

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
          patchelf
        ];

        # for some reason, buildInputs performs some magic an converts them to build dependencies, not runtime dependencies
        # also, non static dependencies (?? how to ensure ??)
        dlopenBuildInputs = with pkgs; [
          dbus

          xorg.libX11
          xorg.libXext
          xorg.xorgproto
          libxkbcommon
          xorg.libICE
          xorg.libXi
          xorg.libXScrnSaver
          xorg.libXcursor
          xorg.libXinerama
          xorg.libXrandr
          xorg.libXxf86vm

          libGL

          pipewire
        ];

        buildInputs = with pkgs; [
          #(libsodium.override { stdenv = pkgs.pkgsStatic.stdenv; })
          #pkgsStatic.libsodium
          libsodium
        ] ++ self.packages.${system}.default.dlopenBuildInputs;

        # TODO: replace with install command
        installPhase = ''
          mkdir -p $out/bin
          mv bin/tomato $out/bin
        '';

        # copied from nixpkgs's SDL2 default.nix
        # SDL is weird in that instead of just dynamically linking with
        # libraries when you `--enable-*` (or when `configure` finds) them
        # it `dlopen`s them at runtime. In principle, this means it can
        # ignore any missing optional dependencies like alsa, pulseaudio,
        # some x11 libs, wayland, etc if they are missing on the system
        # and/or work with wide array of versions of said libraries. In
        # nixpkgs, however, we don't need any of that. Moreover, since we
        # don't have a global ld-cache we have to stuff all the propagated
        # libraries into rpath by hand or else some applications that use
        # SDL API that requires said libraries will fail to start.
        #
        # You can grep SDL sources with `grep -rE 'SDL_(NAME|.*_SYM)'` to
        # list the symbols used in this way.
        # TODO: only patch if building FOR nix (like not for when static building)
        postFixup =
          let
            #rpath = lib.makeLibraryPath (dlopenPropagatedBuildInputs ++ dlopenBuildInputs);
            rpath = nixpkgs.lib.makeLibraryPath (self.packages.${system}.default.dlopenBuildInputs);
          in
          nixpkgs.lib.optionalString (pkgs.stdenv.hostPlatform.extensions.sharedLibrary == ".so") ''
            patchelf --set-rpath "$(patchelf --print-rpath $out/bin/tomato):${rpath}" "$out/bin/tomato"
          '';
      };

      devShells.${system}.default = pkgs.mkShell {
        #inputsFrom = with pkgs; [ SDL2 ];
        buildInputs = [ self.packages.${system}.default ]; # this makes a prebuild tomato available in the shell, do we want this?

        packages = with pkgs; [
          cmake
          pkg-config
        ];

        shellHook = "echo hello to tomato dev shell!";
      };

      apps.default = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/tomato";
      };
    }
  );
}
