#!/bin/bash -e

# Parse command-line options

# Option strings
ARG_LIST=("env" "build")

#i read the options
OPTS=$(getopt \
    --options "" \
    --long "$(printf "%s:," "${ARG_LIST[@]}")" \
    --name "$(basename "$0")"\
    -- "$@"
)

echo $OPTS

if [ $? != 0 ] ; then echo "Failed to parse options...exiting." >&2 ; exit 1 ; fi

# target environment is mandatory
targetEnv=""
# build type is optional, Release is default
buildType="Release"

eval set --$OPTS

# extract options and their arguments into variables
while [[ $# -gt 0  ]] ; do
	case "$1" in
		-e | --env )
			targetEnv="$2"
			echo $targetEnv
			if ! [[ $targetEnv =~ ^(MAINNET|TESTNET)$ ]]; then
				echo "Target environment must be MAINNET or TESTNET"
				exit 1;
			fi
			shift 2
			;;
		-b | --build )
			buildType=$2
			echo $targetEnv
			if ! [[ $buildType =~ ^(Release|Debug)$ ]]; then
				echo "Build type must be Release or Debug"
				exit 1;
			fi
			shift 2
			;;
		*)
			break
			;;
	esac
done

# check mandatory argument
if [ -z $targetEnv ]; then
        echo "Target environment must be set <MAINNET|TESTNET>"
        exit 1;
fi

# name the docker image
imageName="onegram/onegram-testnet"
[ $targetEnv == "MAINNET" ] && imageName="onegram/onegram-core"

# build docker image
docker build -t $imageName --build-arg TARGET_ENV=$targetEnv --build-arg BUILD=$buildType --build-arg REVISION_SHA=`git rev-parse --short HEAD` --build-arg REVISION_TIMESTAMP=`git log -1 --format=%at` -f Dockerfile.OGC .
