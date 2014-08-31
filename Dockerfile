FROM ubuntu:12.04

MAINTAINER dccrazyboy "dccrazyboy@gmail.com"

# choose a faster mirror, see http://t.cn/zWYrzCE
# RUN echo "deb mirror://mirrors.ubuntu.com/mirrors.txt precise main restricted universe multiverse" > /etc/apt/sources.list
# RUN echo "deb mirror://mirrors.ubuntu.com/mirrors.txt precise-updates main restricted universe multiverse" >> /etc/apt/sources.list
# RUN echo "deb mirror://mirrors.ubuntu.com/mirrors.txt precise-backports main restricted universe multiverse" >> /etc/apt/sources.list
# RUN echo "deb mirror://mirrors.ubuntu.com/mirrors.txt precise-security main restricted universe multiverse" >> /etc/apt/sources.list

RUN apt-get update

RUN apt-get install -y --force-yes build-essential

RUN apt-get install -y wget
RUN apt-get install -y git
RUN apt-get install -y make
RUN apt-get install -y tree
RUN apt-get install -y gdb
RUN apt-get install -y htop
RUN wget -c 'http://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.bz2/download' -O /usr/include/boost_1_56_0.tar.bz2
RUN tar --bzip2 -xf /usr/include/boost_1_56_0.tar.bz2 -C /usr/include/
RUN mv /usr/include/boost_1_56_0/boost /usr/include
RUN rm -rf /usr/include/boost_1_56_0
RUN rm -rf /usr/include/boost_1_56_0.tar.bz2
