#!/bin/bash
#
# Build a .deb package for xnec2c inside a Debian/Ubuntu container.
#
# Usage: build-deb.sh
#
# Expects the source tree to be mounted at /build/xnec2c.
# Produces a .deb in /build/xnec2c/artifacts/.
#
set -euo pipefail

SRC=/build/xnec2c
OUT="$SRC/artifacts"
mkdir -p "$OUT"

cd "$SRC"

# Allow git operations in the mounted source tree (dubious ownership in containers).
if command -v git >/dev/null 2>&1; then
    git config --global --add safe.directory "$SRC" 2>/dev/null || true
fi

# Determine version from git describe or AC_INIT.
if command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    GIT_COUNT=$(git rev-list --count HEAD 2>/dev/null || echo 0)
    GIT_SHORT=$(git rev-parse --short HEAD 2>/dev/null || echo unknown)
    AC_VER=$(grep -m1 'AC_INIT' configure.ac | sed 's/.*\[\([0-9.]*\)\].*/\1/')
    VERSION="${AC_VER}.r${GIT_COUNT}.g${GIT_SHORT}"
else
    VERSION=$(grep -m1 'AC_INIT' configure.ac | sed 's/.*\[\([0-9.]*\)\].*/\1/')
fi

# Detect distro for the .deb filename.
DISTRO=$(grep -m1 '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"')
CODENAME=$(grep -m1 '^VERSION_CODENAME=' /etc/os-release | cut -d= -f2 | tr -d '"' || echo "$DISTRO")
ARCH=$(dpkg --print-architecture)

PKG_NAME="xnec2c"
DEB_NAME="${PKG_NAME}_${VERSION}-1_${ARCH}_${DISTRO}-${CODENAME}.deb"

echo "=== Building $DEB_NAME ==="

# Build the source.
./autogen.sh
./configure --prefix=/usr
make -j"$(nproc)"

# Install into a staging directory.
STAGE="$SRC/stage"
rm -rf "$STAGE"
mkdir -p "$STAGE"
make DESTDIR="$STAGE" install

# Build the .deb using dpkg-deb directly (no debian/ control files needed).
# Create the DEBIAN control directory.
mkdir -p "$STAGE/DEBIAN"

cat > "$STAGE/DEBIAN/control" << EOF
Package: ${PKG_NAME}
Version: ${VERSION}-1
Section: hamradio
Priority: optional
Architecture: ${ARCH}
Depends: libgtk-3-0, libgsl27, libepoxy0, libgl1, libc6
Maintainer: 9M2PJU <9m2pju@hamradio.my>
Description: GTK+ Antenna EM Modeling Client (9M2PJU fork)
 Xnec2c is a high-performance multi-threaded electromagnetic simulation
 package to model antenna near- and far-field radiation patterns for
 Linux and UNIX operating systems. The original FORTRAN version of NEC2
 was ported to C by Neoklis Kyriazis, 5B4AZ and released as nec2c. Later
 he wrote xnec2c, a graphical interface for ease of use with many more
 features:
  Multi-threading operation on SMP machines
  On-demand Calculation
  Built-in NEC2 input file editor
  Accelerated Linear Algebra Support
  Interactive Operation
  User Interface
  Color Coding
  and much more.
Homepage: https://github.com/9M2PJU/xnec2c
EOF

# Post-install script for desktop/mime integration.
cat > "$STAGE/DEBIAN/postinst" << 'EOF'
#!/bin/sh
set -e
if command -v update-mime-database >/dev/null 2>&1; then
    update-mime-database /usr/share/mime || true
fi
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f /usr/share/icons/hicolor || true
fi
EOF
chmod 755 "$STAGE/DEBIAN/postinst"

# Pre-remove script.
cat > "$STAGE/DEBIAN/prerm" << 'EOF'
#!/bin/sh
set -e
EOF
chmod 755 "$STAGE/DEBIAN/prerm"

# Build the .deb.
dpkg-deb --build --root-owner-group "$STAGE" "$OUT/$DEB_NAME"

echo "=== Built: $OUT/$DEB_NAME ==="
ls -la "$OUT/$DEB_NAME"
