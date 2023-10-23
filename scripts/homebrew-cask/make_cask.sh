#!/usr/bin/env bash

if [[ $OSTYPE != 'darwin'* ]]; then
	echo 'Error: this script must be run on macos'
	exit 1
fi

VERSION=$1
CWD="$PWD"

if [[ $VERSION == '' ]]; then
	echo 'Error: please provide a version # (ie. 0.0.1)'
	exit 1
fi

if [[ "$VERSION" =~ ^[0-9]\.[0-9]\.[0-9]{1,2}$ ]]; then
	echo -n ""
else
	echo 'Error: please provide a version # in the following format: #.#.#'
	exit 1
fi

_get_file_from_github() {
	FILE=$1
	OUTPUT=$2
	curl -LJO "https://github.com/chalet-org/chalet/releases/download/v$VERSION/$FILE"
	if  [[ $? != 0 ]]; then
		exit 1
	fi
}

echo "'$VERSION'"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "$SCRIPT_DIR"

TMP_DIR="$SCRIPT_DIR/cask_temp"

mkdir $TMP_DIR
cd "$TMP_DIR"

FILE_ARM64="chalet-arm64-apple-darwin.zip"
FILE_X86="chalet-x86_64-apple-darwin.zip"

_get_file_from_github "$FILE_ARM64"
_get_file_from_github "$FILE_X86"

SHA_ARM64=$(shasum -a 256 "$FILE_ARM64" | cut -d ' ' -f1)
SHA_X86=$(shasum -a 256 "$FILE_X86" | cut -d ' ' -f1)

echo "'$SHA_ARM64'"
echo "'$SHA_X86'"

cd "$SCRIPT_DIR"

cat > "$VERSION.csv" << END
arm64,$SHA_ARM64
x86_64,$SHA_X86
END

rm -rf "$TMP_DIR"

exit 0