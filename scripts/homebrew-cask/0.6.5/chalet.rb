# Chalet Homebrew Cask (WIP)
#
cask "chalet" do
	version "0.6.5"
	sha256 arm: "287ba2c87d62d117d7510337006b513f8c95640956449b13930f1ba9ef64a7ab",
		   intel: "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
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
