#!/bin/bash

cwd=$PWD

if [[ $OSTYPE == 'linux-gnu'* ]]; then
	PLATFORM=linux
fi

if [[ $PLATFORM == '' ]]; then
	echo 'Error: This script must only be run on linux.'
	exit 1
fi

WHICH_DPKG_DEB=$(which dpkg-deb)
RESULT=$?

if [[ $RESULT != 0 ]]; then
	echo 'Error: This script requires dpkg-deb.'
	exit 1
fi

WHICH_DH_LINK=$(which dh_link)
RESULT=$?

if [[ $RESULT != 0 ]]; then
	echo 'Error: This script requires dh_link (part of debhelper).'
	exit 1
fi

CHALET_VERSION=$1
CHALET_ARCHITECTURE=$2

if [[ $CHALET_VERSION == '' || $CHALET_ARCHITECTURE == '' ]]; then
	echo 'Error: This script expects 2 arguments for version & architecture.'
	exit 1
fi

PLATFORM_LINUX_PATH="$cwd/platform/linux/deb"

if [[ -d "platform/linux/deb" ]]; then
else
	echo 'Please run this script from the root of the chalet repository'
	exit 1
fi

DIST_FOLDER="$cwd/dist"
OUT_DEP_DIR="chalet_${CHALET_VERSION}_${CHALET_ARCHITECTURE}"
PKG_ROOT="$DIST_FOLDER/$OUT_DEP_DIR"

if [[ -d "$DIST_FOLDER" ]]; then
else
	chalet-debug bundle
fi

if [[ -d "$PKG_ROOT" ]]; then
	rm -rf "$PKG_ROOT"
fi

PKG_DEBIAN="$PKG_ROOT/debian"
PKG_BIN="$PKG_ROOT/usr/bin"
PKG_OPT="$PKG_ROOT/opt/chalet"
PKG_COMPLETIONS="$PKG_ROOT/usr/share/bash-completion/completions"

mkdir -p "$PKG_ROOT"
mkdir -p "$PKG_DEBIAN"
mkdir -p "$PKG_BIN"
mkdir -p "$PKG_OPT"
mkdir -p "$PKG_COMPLETIONS"

cp "$DIST_FOLDER/chalet-completion.sh" "$PKG_COMPLETIONS/chalet-completion.bash"
cp "$DIST_FOLDER/chalet" "$PKG_OPT"
cp "$DIST_FOLDER/LICENSE.txt" "$PKG_OPT"
cp "$DIST_FOLDER/README.md" "$PKG_OPT"

cat << END > "$PKG_DEBIAN/control"
Package: chalet
Version: $CHALET_VERSION
Architecture: any-$CHALET_ARCHITECTURE
Essential: no
Priority: optional
Depends: ninja-build,cmake,base-devel
Maintainer: Cosmic Road Interactive, LLC.
Description: A JSON-based project & build tool
END

cp "$PLATFORM_LINUX_PATH/preinst" "$PKG_DEBIAN"
cp "$PLATFORM_LINUX_PATH/postinst" "$PKG_DEBIAN"

cd "$PKG_ROOT"
dh_link opt/chalet/chalet usr/bin/chalet

cd "$DIST_FOLDER"

dpkg-deb --build "$OUT_DEP_DIR/"

cd "$cwd"

echo "Completed."

exit 0
