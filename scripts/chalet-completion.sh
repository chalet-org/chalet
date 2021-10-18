#/usr/bin/env bash

_chalet_completions()
{
	_CMDS=($COMP_LINE)
	cur="${_CMDS[COMP_CWORD]}"
	prev="${_CMDS[COMP_CWORD-1]}"

	case "${prev}" in
	-c|--configuration)
		COMPREPLY=($(compgen -W "$(chalet query configurations)" -- $cur))
		;;
	-t|--toolchain)
		COMPREPLY=($(compgen -W "$(chalet query all-toolchains)" -- $cur))
		;;
	-a|--arch)
		COMPREPLY=($(compgen -W "$(chalet query architectures)" -- $cur))
		;;
	query)
		COMPREPLY=($(compgen -W "$(chalet query list-names)" -- $cur))
		;;
	theme)
		COMPREPLY=($(compgen -W "$(chalet query theme-names)" -- $cur))
		;;
	get|getkeys|set|unset)
		_CMDS[COMP_CWORD-1]=getkeys
		_CMDS[COMP_CWORD]="${cur//\\\\./\\.}"
		_RESP=$(${_CMDS[@]})
		COMPREPLY=($(compgen -W "${_RESP//\\\\./\\\\\\\\.}" -- $cur))
		;;
	*)
		COMPREPLY=($(compgen -W "$(chalet query commands)" -- $cur))
		;;
	esac
}

complete -o nospace -F _chalet_completions chalet
