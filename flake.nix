{
  description = "tomato flake";
  # bc flakes and git submodules dont really like each other, you need to
  # append '.?submodules=1' to the nix commands.

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/release-23.11";
    flake-utils.url = "github:numtide/flake-utils";
    nlohmann-json = {
      url = "github:nlohmann/json/v3.11.3"; # TODO: read version from file
      flake = false;
    };
    sdl3 = {
      url = "github:libsdl-org/SDL/0429f5d6a36fc35b551bcc2acd4a40c2db6dab82"; # keep in sync this cmake
      flake = false;
    };
    sdl3_image = {
      url = "github:libsdl-org/SDL_image/a45d6e5b84ccc0f3faae6ba7d561709ed600eee7";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, nlohmann-json, sdl3, sdl3_image }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs { inherit system; };
      stdenv = (pkgs.stdenvAdapters.keepDebugInfo pkgs.stdenv);
    in {
      #packages.default = pkgs.stdenv.mkDerivation {
      packages.default = stdenv.mkDerivation {
        pname = "tomato";
        version = "0.0.0";

        src = ./.;
        submodules = 1; # does nothing

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

          # sdl3_image:
          libpng
          libjpeg
          libjxl
          libavif
          #libwebp # still using our own loader
        ];

        buildInputs = with pkgs; [
          #(libsodium.override { stdenv = pkgs.pkgsStatic.stdenv; })
          #pkgsStatic.libsodium
          libsodium
        ] ++ self.packages.${system}.default.dlopenBuildInputs;

        cmakeFlags = [
          "-DTOMATO_ASAN=OFF"
          "-DCMAKE_BUILD_TYPE=RelWithDebInfo"

          "-DFETCHCONTENT_SOURCE_DIR_JSON=${nlohmann-json}" # we care about the version
          # TODO: use package instead
          "-DFETCHCONTENT_SOURCE_DIR_ZSTD=${pkgs.zstd.src}" # we dont care about the version (we use 1.4.x features)
          "-DFETCHCONTENT_SOURCE_DIR_LIBWEBP=${pkgs.libwebp.src}"
          "-DFETCHCONTENT_SOURCE_DIR_SDL3=${sdl3}"
          "-DFETCHCONTENT_SOURCE_DIR_SDL3_IMAGE=${sdl3_image}"
        ];

        # TODO: replace with install command
        installPhase = ''
          mkdir -p $out/bin
          mv bin/tomato $out/bin
        '';

        dontStrip = true; # does nothing

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

      #packages.debug = pkgs.enableDebugging self.packages.${system}.default;

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
