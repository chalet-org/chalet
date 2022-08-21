#/usr/bin/env bash

_chalet_completions()
{
	_CMDS=($COMP_LINE)
	cur="${_CMDS[COMP_CWORD]}"
	prev="${_CMDS[COMP_CWORD-1]}"

	case "${prev}" in
	run|buildrun|options.runTarget)
		COMPREPLY=($(compgen -W "$(chalet query all-run-targets)" -- $cur))
		;;
	-c|--configuration|options.configuration)
		COMPREPLY=($(compgen -W "$(chalet query configurations)" -- $cur))
		;;
	-t|--toolchain|options.toolchain)
		COMPREPLY=($(compgen -W "$(chalet query all-toolchains)" -- $cur))
		;;
	-a|--arch|options.architecture)
		COMPREPLY=($(compgen -W "$(chalet query architectures)" -- $cur))
		;;
	export)
		COMPREPLY=($(compgen -W "$(chalet query export-kinds)" -- $cur))
		;;
	query)
		COMPREPLY=($(compgen -W "$(chalet query list-names)" -- $cur))
		;;
	theme)
		COMPREPLY=($(compgen -W "$(chalet query theme-names)" -- $cur))
		;;
	-b|--build-strategy|toolchains.*.strategy)
		COMPREPLY=($(compgen -W "$(chalet query build-strategies)" -- $cur))
		;;
	toolchains.*.buildPathStyle)
		COMPREPLY=($(compgen -W "$(chalet query build-path-styles)" -- $cur))
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
complete -o nospace -F _chalet_completions chalet-debug
