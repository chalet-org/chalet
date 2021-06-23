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
	'subprocess'
	'ust'
	'hash-library'
)

repositories=(
	'p-ranav/argparse'
	'catchorg/Catch2'
	'hanickadot/compile-time-regular-expressions'
	'fmtlib/fmt'
	'gulrak/filesystem'
	'andrew-r-king/glob'
	'andrew-r-king/json-schema-validator'
	'nlohmann/json'
	'andrew-r-king/subprocess'
	'MisterTea/UniversalStacktrace'
	'andrew-r-king/hash-library'
)
tags=(
	'64dd67c7587ec36d1fc39a0f03619ea219968a95'
	'v2.x'
	'f6eba2662aac0284c1cb5c0dc19c72f47820b17b'
	'8.0.0'
	'v1.5.6'
	'master'
	'master'
	'v3.9.1'
	'master'
	'e6ca24525f28556dd4847443b07dd4b9a4affa9d'
	'master'
)

commits=(
	1
	0
	1
	0
	0
	0
	0
	0
	0
	1
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
