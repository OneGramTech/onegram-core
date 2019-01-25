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

# target environment is optional, all targets are built if not parameter not provided
targetEnv="ALL"
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


if [[ $targetEnv =~ ^(MAINNET|ALL)$ ]]; then
	imageName="onegram/onegram-core"

	# build MAINNET docker image
	docker build -t $imageName --build-arg TARGET_ENV=MAINNET --build-arg BUILD=$buildType --build-arg REVISION_SHA=`git rev-parse --short HEAD` --build-arg REVISION_TIMESTAMP=`git log -1 --format=%at` -f Dockerfile.OGC .
fi

if [[ $targetEnv =~ ^(TESTNET|ALL)$ ]]; then
	imageName="onegram/onegram-testnet"

	# build TESTNET docker image
	docker build -t $imageName --build-arg TARGET_ENV=TESTNET --build-arg BUILD=$buildType --build-arg REVISION_SHA=`git rev-parse --short HEAD` --build-arg REVISION_TIMESTAMP=`git log -1 --format=%at` -f Dockerfile.OGC .
fi
