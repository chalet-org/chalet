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
	get|getkeys|set|unset)
		_CMDS[COMP_CWORD-1]=getkeys
		_CMDS[COMP_CWORD]="${_CMDS[COMP_CWORD]//\\\\./\\.}"
		_RESP=$(${_CMDS[@]})
		COMPREPLY=($(compgen -W "${_RESP//\\\\./\\\\\\\\.}"))
	;;
	*)
		COMPREPLY=($(compgen -W "$(chalet list --type commands)" -- "$cur"))
	;;
	esac
}

complete -o nospace -F _chalet_completions chalet
