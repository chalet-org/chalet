#!/usr/bin/env bash

OUTDIR="vendor"

vendors=(
	'argparse'
	'catch2'
	'ctre'
	'fmt'
	'ghc'
	'glob'
	'json-schema-validator'
	'nlohmann'
	'thread-pool'
)

repositories=(
	'chalet-org/argparse'
	'catchorg/Catch2'
	'hanickadot/compile-time-regular-expressions'
	'fmtlib/fmt'
	'gulrak/filesystem'
	'chalet-org/glob'
	'chalet-org/json-schema-validator'
	'nlohmann/json'
	'chalet-org/thread-pool'
)
tags=(
	'master'
	'v2.x'
	'82ce68b9e2b705a83d107d842f51fde42a03133b'
	'8.0.0'
	'v1.5.6'
	'master'
	'master'
	'v3.10.2'
	'v2.0.0'
)

commits=(
	0
	0
	1
	0
	0
	0
	0
	0
	0
)

printf "Fetching dependencies into '$OUTDIR'...\n\n"

# git clone [--quiet] -b [tag|branch] [--single-branch|--depth 1] --config advice.detatchedHead=false (repo) (destination)
color='\033[1;35m'
color_reset='\033[0m'
fetching_symbol='➥'

# use --single-branch if pulling specific commit
for idx in 0 1 2 3 4 5 6 7 8 9 10; do
	vendor=${vendors[$idx]}
	repo=${repositories[$idx]}
	tag=${tags[$idx]}
	single_commit=${commits[$idx]}
	path="$OUTDIR/$vendor"

	if [[ ! -d "$path" ]]; then
		printf "$color$fetching_symbol  Fetching: $repo ($tag)$color_reset\n"
		mkdir -p "$path"

		if [[ "$single_commit" == "1" ]]; then
			git clone --quiet --single-branch --config "advice.detachedHead=false" "https://github.com/$repo.git" "$path"
			git -C "$path" reset --quiet --hard "$tag"
		else
			git clone --quiet -b "$tag" --depth 1 --config "advice.detachedHead=false" "https://github.com/$repo.git" "$path"
		fi

		rm -rf "$path/.git/"
	fi
done
printf "\n"
