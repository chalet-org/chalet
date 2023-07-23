#/usr/bin/env bash

_get_toolchain()
{
	# Note: this method works in sh, bash & zsh
	local _L=($1)
	for ((idx = 1; idx <= ${#_L[@]}; idx++)); do
		local cur=${_L[$idx]}
		if [[ $cur == "-t" || $cur == "--toolchain" ]]; then
			if [[ ${idx+1} < ${#_L[@]} ]]; then
				_TOOLCHAIN=${_L[$idx+1]}
			fi
			return 0
		fi
	done
}

_chalet_completions()
{
	local _CMDS=($COMP_LINE)
	local cur="${_CMDS[COMP_CWORD]}"
	local prev="${_CMDS[COMP_CWORD-1]}"

	case "${cur}" in
	-*)
		COMPREPLY=($(compgen -W "$(chalet query options)" -- $cur))
		;;
	*)
		case "${prev}" in
		run|buildrun)
			COMPREPLY=($(compgen -W "$(chalet query all-run-targets)" -- $cur))
			;;
		build|rebuild|options.lastTarget)
			COMPREPLY=($(compgen -W "$(chalet query all-build-targets)" -- $cur))
			;;
		-c|--configuration|options.configuration)
			COMPREPLY=($(compgen -W "$(chalet query configurations)" -- $cur))
			;;
		-t|--toolchain|options.toolchain)
			COMPREPLY=($(compgen -W "$(chalet query all-toolchains)" -- $cur))
			;;
		-a|--arch|options.architecture)
			_get_toolchain "$COMP_LINE"
			COMPREPLY=($(compgen -W "$(chalet query architectures $_TOOLCHAIN)" -- $cur))
			unset _TOOLCHAIN
			;;
		-b|--build-strategy|toolchains.*.strategy)
			COMPREPLY=($(compgen -W "$(chalet query build-strategies)" -- $cur))
			;;
		-p|--build-path-style|toolchains.*.buildPathStyle)
			COMPREPLY=($(compgen -W "$(chalet query build-path-styles)" -- $cur))
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
	esac
}

complete -o nospace -F _chalet_completions chalet
complete -o nospace -F _chalet_completions chalet-debug
