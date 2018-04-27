#### List Available Local Images

    > docker images

#### List All Containers

Following command will list all (active and axited) available containers. Use this command to retrieve the container id.

    > docker ps -a

#### Create Docker Hub Account

Sign up to [https://hub.docker.com/](https://hub.docker.com/ "Docker Hub")

#### Login to Docker Account

    > docker login --username=<user_name> --password=<user_password>
    
    > docker build -f Dockerfile.pi3_ubuntu_gcc .
    
    > docker images
    REPOSITORY TAG     IMAGE ID      CREATED     SIZE
    <none>     <none>  724562e5fef7  4 days ago  756MB
    
    > docker tag 724562e5fef7 frankhorv/arm64v8-ubuntu-gcc
    
    > docker images
    REPOSITORY                     TAG     IMAGE ID      CREATED     SIZE
    frankhorv/arm64v8-ubuntu-gcc   latest  724562e5fef7  4 days ago  756MB
    
#### Push an Image into Docker Hub

The following command will upload your image into the Docker Hub repository.

    > docker push frankhorv/arm64v8-ubuntu-gcc

#### Run a Docker Image

Execute an image inside the docker engine (image is automatically downoaded, if not available yet):

    > docker run -it -v /OneGramBuild/:/OneGramBuild/ frankhorv/arm64v8-ubuntu-gcc

#### Start and Attach to an Exited Container

    > docker start <container id>
    > docker attach <container id>
