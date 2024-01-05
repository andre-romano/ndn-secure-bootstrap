
# Overview

This project can be run in either one of the following methods:

## Option 1 - Run project inside Docker (recommended)

Execute the following command:
```bash
bash ./docker-run.sh
```

An ```./ndnSIM``` folder will be created and populated with the required dependencies (ns-3, ndnSIM module, python bindings, bonnmotion, sim_bootsec project inside ```./ns-3/scratch``` folder).

Inside the terminal, type in the following to perform simulations:
```bash
cd ./scratch/sim_bootsec
./run.sh
```

The ```run.sh``` has some simulation paramters that can be changed, please check the file.

## Option 2 - Run project inside Ubuntu VM

### INSTALL Dependencies - Instructions

Following NS3 code was tested under Ubuntu 20.04. It might work using other distros, but it is not guaranteed.

#### INSTALL - NDNSIM 

Execute the following in the bash terminal:
```bash
sudo apt-get update
sudo apt-get -y install vim git wget
sudo apt-get -y install build-essential libsqlite3-dev libboost-all-dev libssl-dev git python3-setuptools castxml
sudo apt-get -y install gir1.2-goocanvas-2.0 gir1.2-gtk-3.0 libgirepository1.0-dev python3-dev python3-gi python3-gi-cairo python3-pip python3-pygraphviz python3-pygccxml sudo pip3 install kiwi

cd ~
mkdir ndnSIM
cd ndnSIM
git clone -b ndnSIM-ns-3.30.1 https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
git clone -b 0.21.0 https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
git clone -b ndnSIM-2.8 --recursive https://github.com/named-data-ndnSIM/ndnSIM.git ns-3/src/ndnSIM
git submodule update --init

cd ns-3
./waf configure --enable-examples
./waf
```

#### INSTALL - BonnMotion mobility generator

Execute the following:
```bash
cd ~
wget -O jdk-21_linux-x64_bin.deb https://download.oracle.com/java/21/latest/jdk-21_linux-x64_bin.deb
sudo apt-get -y install ./jdk-21_linux-x64_bin.deb

wget -O bonnmotion-3.0.1.zip https://sys.cs.uos.de/bonnmotion/src/bonnmotion-3.0.1.zip
unzip bonnmotion-3.0.1.zip
cd bonnmotion-3.0.1
chmod +rx *.sh *.bat ./install
./install
mkdir -p ~/.local/bin
cp bin/bm ~/.local/bin
chmod +rx ~/.local/bin/*
echo "export PATH=\$PATH:\$HOME/.local/bin" >> ~/.bash_aliases
. ~/.bash_aliases
```

#### Setup

Execute the following:
```bash
cd ~/ndnSIM/ns-3/scratch
git clone git@github.com:andre-romano/ndn-secure-bootstrap.git sim_bootsec
chmod +rx -R sim_bootsec/*
```

#### Run experiments

Execute the following:

```bash
cd ~/ndnSIM/ns-3/scratch/ndn-secure-bootstrap
./run.sh
```

You can pass parameters to "run.sh", please check the script for more info


