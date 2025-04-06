{
  description = "tomato flake";
  # bc flakes and git submodules dont really like each other, you need to
  # append '.?submodules=1#' to the nix commands.

  # $ nix bundle --bundler github:ralismark/nix-appimage 'git+file:///home/green/workspace/tox/tomato?submodules=1#'

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/release-24.05";
    flake-utils.url = "github:numtide/flake-utils";
    nlohmann-json = {
      url = "github:nlohmann/json/v3.11.3"; # TODO: read version from file
      flake = false;
    };
    sdl3 = {
      url = "github:libsdl-org/SDL/877399b2b2cf21e67554ed9046410f268ce1d1b2"; # keep in sync with cmake
      flake = false;
    };
    sdl3_image = {
      url = "github:libsdl-org/SDL_image/11154afb7855293159588b245b446a4ef09e574f";
      flake = false;
    };
    plutosvg = {
      url = "git+https://github.com/sammycage/plutosvg?submodules=1&ref=refs/tags/v0.0.6";
      flake = false;
    };
    implot = {
      url = "github:epezent/implot/193b9d8f92c4a437e84182b171f1ae266e72321f";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, nlohmann-json, sdl3, sdl3_image, plutosvg, implot }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs { inherit system; };
      stdenv = (pkgs.stdenvAdapters.keepDebugInfo pkgs.stdenv);
      #stdenv = (pkgs.stdenvAdapters.withCFlags [ "-march=x86-64-v3" ] (pkgs.stdenvAdapters.keepDebugInfo pkgs.stdenv));
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

        # for some reason, buildInputs performs some magic and converts them to build dependencies, not runtime dependencies
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

          libayatana-appindicator

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
          libopus
          libvpx

          zstd

          libwebp

          freetype
        ] ++ self.packages.${system}.default.dlopenBuildInputs;

        cmakeFlags = [
          "-DTOMATO_TOX_AV=ON"
          "-DTOMATO_ASAN=OFF"
          "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
          #"-DCMAKE_BUILD_TYPE=Debug"
          #"-DCMAKE_C_FLAGS:STRING=-Og"
          #"-DCMAKE_CXX_FLAGS:STRING=-Og"

          # TODO: use package instead
          "-DFETCHCONTENT_SOURCE_DIR_JSON=${nlohmann-json}" # we care about the version
          "-DFETCHCONTENT_SOURCE_DIR_SDL3=${sdl3}"
          #"-DFETCHCONTENT_SOURCE_DIR_LIBWEBP=${pkgs.libwebp.src}"
          "-DTOMATO_DEP_LIBWEBP_USE_SYSTEM=ON"
          "-DFETCHCONTENT_SOURCE_DIR_SDL3_IMAGE=${sdl3_image}"
          "-DSDLIMAGE_JXL=ON"
          "-DFETCHCONTENT_SOURCE_DIR_PLUTOSVG=${plutosvg}" # not in pkgs
          "-DFETCHCONTENT_SOURCE_DIR_IMPLOT=${implot}" # could use pkgs.implot.src for outdated version
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

        shellHook = ''
          echo hello to tomato dev shell!
          export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/run/opengl-driver/lib
          '';
      };

      apps.default = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/tomato";
      };
    }
  );
}
