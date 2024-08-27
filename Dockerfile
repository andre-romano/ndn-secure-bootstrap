FROM docker.io/andreromano/ndnsim:2.8
RUN apt-get update && \
    apt-get install -y --no-install-recommends wget tar unzip python3 && \
    mkdir -p /opt && \
    cd /opt && \
    wget -O jdk_x64.tar.gz https://download.oracle.com/java/21/latest/jdk-21_linux-x64_bin.tar.gz && \
    tar axvf jdk_x64.tar.gz && \
    rm -f jdk_x64.tar.gz && \
    export JDK_PATH=$(ls -d /opt/jdk-*) && \
    chmod 755 -R $JDK_PATH && \
    ln -sf $JDK_PATH/bin/java  /usr/bin && \
    ln -sf $JDK_PATH/bin/javac /usr/bin && \
    wget -O bonnmotion.zip https://sys.cs.uos.de/bonnmotion/src/bonnmotion-3.0.1.zip && \
    unzip bonnmotion.zip && \
    rm bonnmotion.zip && \
    export BONNMOTION_PATH=$(ls -d /opt/bonnmotion-*) && \
    cd bonnmotion-3.0.1 && \
    chmod 755 -R . && \
    bash -c "./install <<< \"$JDK_PATH/bin\" " && \
    ln -sf /opt/bonnmotion-3.0.1/bin/bm /usr/bin 
