# Chalet Homebrew Cask (WIP)
#
cask "chalet" do
	version "0.6.6"
	sha256 arm: "d72cb330d464e7132334c54ddf469d93e6e65fc0eef93391f0e48c4a1335fe2d",
		   intel: "e6791d2e04c66c51c1006f078fb6ef54a5cb630d2cbc855aa2758e8e2205edee"
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
