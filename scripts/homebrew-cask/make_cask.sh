#!/usr/bin/env bash

if [[ $OSTYPE != 'darwin'* ]]; then
	echo 'Error: this script must be run on macos'
	exit 1
fi

VERSION=$1
CWD="$PWD"

echo "'$VERSION'"

if [[ $VERSION == '' ]]; then
	echo 'Error: please provide a version # (ie. 0.0.1)'
	exit 1
fi

if [[ "$VERSION" =~ ^[0-9]\.[0-9]\.[0-9]$ ]]; then
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

rm -rf "$VERSION"
mkdir "$VERSION"

cat > "$VERSION/chalet.rb" << END
# Chalet Homebrew Cask (WIP)
#
cask "chalet" do
	version "$VERSION"
	sha256 arm: "$SHA_ARM64",
		   intel: "$SHA_X86"
	arch arm: "arm64",
		 intel: "x86_64"

	url "https://github.com/chalet-org/chalet/releases/download/v#{version}/chalet-#{arch}-apple-darwin.zip"
	name "Chalet"
	desc "A cross-platform project format & build tool for C/C++"
	homepage "https://www.chalet-work.space"

	livecheck do
		url :stable
		regex(/^v?(\d+(?:\.\d+)+)$/i)
	end

	auto_updates true
	depends_on macos: ">= :big_sur"

	binary "chalet"
end
END

cat > "$VERSION.csv" << END
arm,$SHA_ARM64
intel,$SHA_X86
END

rm -rf "$TMP_DIR"

exit 0