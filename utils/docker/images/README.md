# Content

Dockerfiles and scripts placed in this directory are indended to be used as
developement process vehicles and part of continuous integration process.

Images built out of those recipes may by used with docker or podman as
developement environment.
Only those used on travis are fully tested on a daily basis.
In case of any problem, patches and github issues are welcome.

# How to build docker image

```sh
docker build --build-arg https_proxy=https://proxy.com:port --build-arg http_proxy=http://proxy.com:port -t pmemkv:debian-unstable -f ./Dockerfile.debian-unstable .
```

# How to use docker image

To run build and tests on local machine on docker:

```sh
docker run --network=bridge --shm-size=4G -v /your/workspace/path/:/opt/workspace:z -w /opt/workspace/ -e CC=clang -e CXX=clang++ -e PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig -it pmemkv:debian-unstable /bin/bash
```

To get strace working, add to docker commandline

```sh
 --cap-add SYS_PTRACE
```
