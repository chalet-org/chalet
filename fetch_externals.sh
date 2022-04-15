#!/usr/bin/env bash

OUTDIR="external"

libraries=(
	'catch2'
	'fmt'
	'json-schema-validator'
	'nlohmann'
)

repositories=(
	'catchorg/Catch2'
	'fmtlib/fmt'
	'chalet-org/json-schema-validator'
	'nlohmann/json'
)
tags=(
	'v2.x'
	'8.1.1'
	'master'
	'v3.10.2'
)

commits=(
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
	library=${libraries[$idx]}
	repo=${repositories[$idx]}
	tag=${tags[$idx]}
	single_commit=${commits[$idx]}
	path="$OUTDIR/$library"

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
