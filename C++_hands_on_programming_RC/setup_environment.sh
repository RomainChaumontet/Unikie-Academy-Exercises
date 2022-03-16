#!/usr/bin/sh

#Try to find Bazel
BAZEL_ALREADY_INSTALLED=false
if ! [ -x "$(command -v bazel git)" ]; then
    BAZEL_ALREADY_INSTALLED=false
else
    BAZEL_ALREADY_INSTALLED=true

fi
#echo $BAZEL_ALREADY_INSTALLED


#Try to find g++
GPLUSPLUS_ALREADY_INSTALLED=false
if ! [ -x "$(command -v g++ git)" ]; then
    GPLUSPLUS_ALREADY_INSTALLED=false
else
    GPLUSPLUS_ALREADY_INSTALLED=true
fi
#echo $GPLUSPLUS_ALREADY_INSTALLED

#Try to find UNZIP
UNZIP_ALREADY_INSTALLED=true
# if ! [ -x "$(command -v unzip git)" ]; then
#     UNZIP_ALREADY_INSTALLED=false
# else
#     UNZIP_ALREADY_INSTALLED=true
# fi
#echo $UNZIP_ALREADY_INSTALLED

#Try to find ZIP
ZIP_ALREADY_INSTALLED=true
# if ! [ -x "$(command -v zip git)" ]; then
#     ZIP_ALREADY_INSTALLED=false
# else
#     ZIP_ALREADY_INSTALLED=true
# fi
#echo $ZIP_ALREADY_INSTALLED

#Try to find git
GIT_ALREADY_INSTALLED=true
# if ! [ -x "$(command -v git git)" ]; then
#     GIT_ALREADY_INSTALLED=false
# else
#     GIT_ALREADY_INSTALLED=true
# fi
#echo $GIT_ALREADY_INSTALLED

#install programs missing
if  ! [ $BAZEL_ALREADY_INSTALLED ] ||\
    ! [ $ZIP_ALREADY_INSTALLED ] ||\
    ! [ $UNZIP_ALREADY_INSTALLED ] ||\
    ! [ $GPLUSPLUS_ALREADY_INSTALLED ] ||\
    ! [ $GIT_ALREADY_INSTALLED ]]; then

    apt update

    if ! [ $ZIP_ALREADY_INSTALLED ]; then
        apt install zip
    fi

    if ! [ $UNZIP_ALREADY_INSTALLED ]; then
        apt install unzip
    fi

    if ! [ $GPLUSPLUS_ALREADY_INSTALLED ]; then
        apt install g++
    fi

    if ! [ $GIT_ALREADY_INSTALLED ]; then
        apt install git
    fi

    if ! [ $BAZEL_ALREADY_INSTALLED ]; then
        sudo apt install apt-transport-https curl gnupg
        curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
        sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
        echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
        apt install bazel
    fi
fi


chmod +x ./build_and_run_tests.sh 














