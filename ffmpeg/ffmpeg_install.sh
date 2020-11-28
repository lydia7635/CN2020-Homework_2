#!/bin/bash
set -o errexit

# Download ffmpeg-3.3
mkdir ffmpeg -p
wget http://ffmpeg.org/releases/ffmpeg-3.3.tar.bz2 -O ffmpeg/ffmpeg-3.3.tar.bz2

# Untar ffmpeg-3.3
cd ffmpeg
tar xvf ffmpeg-3.3.tar.bz2
cd ffmpeg-3.3

# Build ffmpeg
./configure --enable-pic --enable-shared
make

# Prepare the output package
mkdir ffmpeg-lib -p
mkdir ffmpeg-lib/lib -p
mkdir ffmpeg-lib/inc -p
libarr=("avcodec" "avdevice" "avfilter" "avformat" "avutil" "swresample" "swscale")
for lib in ${libarr[@]}; do
    echo "Copy lib and header for $lib"
    cp lib${lib}/lib${lib}.so* ffmpeg-lib/lib/ -av
    mkdir -p ffmpeg-lib/inc/lib${lib}
    cp lib${lib}/*.h ffmpeg-lib/inc/lib${lib}/
done

# Create ffmpeg artifact
tar -cpzvf ffmpeg-lib.tar.gz ffmpeg-lib