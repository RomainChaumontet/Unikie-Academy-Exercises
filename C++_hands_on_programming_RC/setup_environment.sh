#!/usr/bin/sh
set -x
#ask for sudo et elevate privilege
[ "$(whoami)" != "root" ] && exec sudo -- "$0" "$@"

#Force time update
sudo timedatectl set-ntp off
sudo timedatectl set-ntp on

#Try to find Bazel
BAZEL_NEED_TO_BE_INSTALLED=true
if ! [ -x "$(command -v bazel)" ]; then
    BAZEL_NEED_TO_BE_INSTALLED=true
else
    BAZEL_NEED_TO_BE_INSTALLED=false

fi
#echo $BAZEL_NEED_TO_BE_INSTALLED


#Try to find g++
GPLUSPLUS_NEED_TO_BE_INSTALLED=true
if ! [ -x "$(command -v g++)" ]; then
    GPLUSPLUS_NEED_TO_BE_INSTALLED=true
else
    GPLUSPLUS_NEED_TO_BE_INSTALLED=false
fi
#echo $GPLUSPLUS_NEED_TO_BE_INSTALLED

#Try to find git
GIT_NEED_TO_BE_INSTALLED=true
if ! [ -x "$(command -v git)" ]; then
     GIT_NEED_TO_BE_INSTALLED=true
else
     GIT_NEED_TO_BE_INSTALLED=false
fi
#echo $GIT_NEED_TO_BE_INSTALLED

#install programs missing
if  $BAZEL_NEED_TO_BE_INSTALLED || $GPLUSPLUS_NEED_TO_BE_INSTALLED || $GIT_NEED_TO_BE_INSTALLED
then
    sudo apt update
    if $GPLUSPLUS_NEED_TO_BE_INSTALLED
    then
        apt install g++
    fi
    if $GIT_NEED_TO_BE_INSTALLED
    then
        apt install git
    fi
    if $BAZEL_NEED_TO_BE_INSTALLED
    then
        sudo apt install apt-transport-https curl gnupg
        curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
        sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
        echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
        sudo apt update && sudo apt install bazel
    fi
fi

chmod +x ./build_and_run_tests.sh 

set +x












