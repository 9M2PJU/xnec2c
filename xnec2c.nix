{ pkgs ? import <nixpkgs> {} }:

let
  xnec2c-git = pkgs.xnec2c.overrideAttrs (old: {
    version = "5.x-unstable";
    src = pkgs.fetchFromGitHub {
      owner = "KJ7LNW";
      repo = "xnec2c";
      rev = "master";
      hash = "sha256-BS5P6BjRJtb9y6XyHJAtDrEW2Ojq6PiZZbVfviuW8YA=";
    };
    nativeBuildInputs = (old.nativeBuildInputs or []) ++ [ pkgs.intltool ];
    buildInputs = (old.buildInputs or []) ++ [ pkgs.gsl pkgs.libepoxy pkgs.openblas ];

    configureFlags = (old.configureFlags or []) ++ [ "--disable-optimizations" ];
    CFLAGS = "-g -O0";
    dontStrip = true;
    enableParallelBuilding = true;
  });
in
pkgs.mkShell {
  packages = [
    xnec2c-git
    pkgs.gdb
    pkgs.valgrind
    pkgs.kdePackages.massif-visualizer
    pkgs.heaptrack
    # build toolchain for local `./autogen.sh && ./configure && make`
    pkgs.autoconf
    pkgs.automake
    pkgs.libtool
    pkgs.pkg-config
    pkgs.glib
    pkgs.intltool
    pkgs.gsl
    pkgs.libepoxy
    pkgs.openblas
    pkgs.gtk3
  ];

  # Samples:
  #LANG = "ar_EG.UTF-8";
  #LANG = "bn_BD.UTF-8";
  #LANG = "de_DE.UTF-8";
  #LANG = "en_US.UTF-8";
  #LANG = "fa_IR.UTF-8";
  #LANG = "fr_FR.UTF-8";
  #LANG = "he_IL.UTF-8";
  #LANG = "hi_IN.UTF-8";
  #LANG = "ja_JP.UTF-8";
}

