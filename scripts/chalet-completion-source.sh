#!/usr/bin/env sh

_CD=$(dirname -- "$0")
_CURSHELL=$(basename $(readlink /proc/$$/exe))
if [[ $_CURSHELL == "zsh" ]]; then
	source $_CD/chalet-completion.zsh
elif [[ $_CURSHELL == "bash" ]]; then
	source $_CD/chalet-completion.bash
elif [[ $_CURSHELL == "sh" ]]; then
	source $_CD/chalet-completion.sh
else
	echo "chalet-completion-source.sh Error: Unsupported shell \"$_CURSHELL\" - Please open an issue here: https://github.com/chalet-org/chalet/issues"
fi
unset _CURSHELL
unset _CD
