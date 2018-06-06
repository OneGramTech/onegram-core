#!/bin/sh
docker build -t onegram/onegram-core --build-arg BUILD=Release --build-arg REVISION_SHA=`git rev-parse --short HEAD` --build-arg REVISION_TIMESTAMP=`git log -1 --format=%at` -f Dockerfile.OGC .
