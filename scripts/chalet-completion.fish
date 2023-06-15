#/usr/bin/env fish

# Note: This isn't as elegant as the other shells (duplicate hard-coding),
#  but it gets the job done without writing a 2000 line fish script

# If adding to args to this file, be sure to add them to __fish_chalet_needs_subcommand
#  in order to exclude them from the subcommand completion, otherwise, subcommands
#  will show in wherever the new arg is

# This function checks the previous arg for the given list
function __fish_chalet_prev_arg
    set -l _CMDS (commandline -poc)
    set -e _CMDS[1]
    set -l COMP_CWORD (count $_CMDS)
    if test $COMP_CWORD -gt 0
        set -l prev $_CMDS[$COMP_CWORD]
        if contains -- $prev $argv
            return 0
        end
    end
    return 1
end

# This function checks the previous arg for any args not otherwise handled
function __fish_chalet_needs_subcommand
    set -l _CMDS (commandline -poc)
    set -e _CMDS[1]
    set -l COMP_CWORD (count $_CMDS)
    if test $COMP_CWORD -gt 0
        set -l prev $_CMDS[$COMP_CWORD]
        set -l list run buildrun options.lastTarget -c --configuration options.configuration -t --toolchain options.toolchain -a --arch options.architecture -b --build-strategy strategy -p --build-path-style buildPathStyle export query theme get getkeys set unset
        if contains -- $prev $list
            return 1
        end
    end
    return 0
end

# We generate completions for multiple executables, so wrap them in a function
function __fish_generate_completions
    set -l executable $argv[1]

    # Disable file completions
    complete -c $executable -f

    # Subcommands
    complete -c $executable -n "__fish_chalet_needs_subcommand ''" -a "$(chalet query commands)" -d ""

    # Various completions we want
    complete -c $executable -n "__fish_chalet_prev_arg run buildrun" -a "$(chalet query all-run-targets)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg options.lastTarget" -a "$(chalet query all-build-targets)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg -c --configuration options.configuration" -a "$(chalet query configurations)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg -t --toolchain options.toolchain" -a "$(chalet query all-toolchains)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg -a --arch options.architecture" -a "$(chalet query architectures)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg -b --build-strategy" -a "$(chalet query build-strategies)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg -p --build-path-style" -a "$(chalet query build-path-styles)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg export" -a "$(chalet query export-kinds)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg query" -a "$(chalet query list-names)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg theme" -a "$(chalet query theme-names)" -d ""
    complete -c $executable -n "__fish_chalet_prev_arg get getkeys set unset" -a "" -d ""
end

# Setup completions for Release & Debug versions
__fish_generate_completions chalet
__fish_generate_completions chalet-debug
