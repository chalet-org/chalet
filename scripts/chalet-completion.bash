#/usr/bin/env bash

_chalet_completions()
{
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"

	case "${prev}" in
	-c|--configuration)
		COMPREPLY=($(compgen -W "Release Debug RelWithDebInfo MinSizeRel Profile" "${cur}"))
	;;
	-t|--toolchain)
		COMPREPLY=($(compgen -W "msvc-pre msvc apple-llvm llvm gcc" "${cur}"))
	;;
	-a|--arch)
		COMPREPLY=($(compgen -W "auto" "${cur}"))
	;;
	*)
	{
		COMPREPLY=($(compgen -W "$(chalet --help | sed -En '/   chalet/! s/^   ([a-z]+).*$/\1/p')" -- "${cur}"))
	}
	;;
	esac
}

complete -F _chalet_completions chalet
