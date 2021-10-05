#/usr/bin/env bash

_chalet_completions()
{
	_CMDS=($COMP_LINE)
	cur="${_CMDS[COMP_CWORD]}"
	prev="${_CMDS[COMP_CWORD-1]}"


	case "${prev}" in
	-c|--configuration)
		COMPREPLY=($(compgen -W "$(chalet list --type configurations)" -- $cur))
	;;
	-t|--toolchain)
		COMPREPLY=($(compgen -W "$(chalet list --type all-toolchains)" -- $cur))
	;;
	-a|--arch)
		COMPREPLY=($(compgen -W "$(chalet list --type architectures)" -- $cur))
	;;
	get|set|unset)
		_CMDS[COMP_CWORD-1]=getkeys
		COMPREPLY=($(compgen -W "$(${_CMDS[@]})"))
	;;
	*)
		COMPREPLY=($(compgen -W "$(chalet list --type commands)" -- $cur))
	;;
	esac
}

complete -o nospace -F _chalet_completions chalet
