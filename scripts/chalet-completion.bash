#/usr/bin/env bash

_chalet_completions()
{
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"

	case "${prev}" in
	-c|--configuration)
		COMPREPLY=($(compgen -W "$(chalet list --type configurations)" "${cur}"))
	;;
	-t|--toolchain)
		COMPREPLY=($(compgen -W "$(chalet list --type all-toolchains)" "${cur}"))
	;;
	-a|--arch)
		COMPREPLY=($(compgen -W "$(chalet list --type architectures)" "${cur}"))
	;;
	*)
		COMPREPLY=($(compgen -W "$(chalet list --type commands)" -- "${cur}"))
	;;
	esac
}

complete -F _chalet_completions chalet
