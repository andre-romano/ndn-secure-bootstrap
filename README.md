
# Overview

**Summary**
- [Overview](#overview)
  - [Option 1 - Run project inside Docker (recommended)](#option-1---run-project-inside-docker-recommended)
  - [Option 2 - Run project inside Ubuntu VM](#option-2---run-project-inside-ubuntu-vm)
    - [INSTALL Dependencies - Instructions](#install-dependencies---instructions)
      - [INSTALL - NDNSIM](#install---ndnsim)
      - [INSTALL - BonnMotion mobility generator](#install---bonnmotion-mobility-generator)
      - [Setup](#setup)
      - [Run experiments](#run-experiments)
- [NDN Intertrust Design](#ndn-intertrust-design)
  - [Description](#description)
  - [Intrazone communication](#intrazone-communication)
    - [1. Assumptions](#1-assumptions)
    - [2. Bootstrapping](#2-bootstrapping)
      - [2.1. Producer PKI Creation](#21-producer-pki-creation)
      - [2.2. Producer Authentication](#22-producer-authentication)
      - [2.3. Producer Certificate Signing](#23-producer-certificate-signing)
      - [2.4. Update Trust Schema](#24-update-trust-schema)
    - [3. Bootstrapping Overview](#3-bootstrapping-overview)
  - [Interzone communication](#interzone-communication)
    - [1. Assumptions](#1-assumptions-1)
    - [2. Bootstrapping](#2-bootstrapping-1)
      - [2.1. Acquire Zone B self-signed trust anchor (``Tb``)](#21-acquire-zone-b-self-signed-trust-anchor-tb)
      - [2.2. Sign Zone B trust anchor](#22-sign-zone-b-trust-anchor)
      - [2.3. Acquire Zone B validation rules](#23-acquire-zone-b-validation-rules)
      - [2.4. Adapt validation rules](#24-adapt-validation-rules)
      - [2.5. Include adapted Zone B rules in trust schema](#25-include-adapted-zone-b-rules-in-trust-schema)
      - [2.5. Send updated Zone A trust schema to interested parties](#25-send-updated-zone-a-trust-schema-to-interested-parties)
    - [3. Bootstrapping Overview](#3-bootstrapping-overview-1)
    - [4. Example (study case)](#4-example-study-case)


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

The ```run.sh``` has some simulation parameters that can be changed, please check the file.

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
unzip bonnmotion-3.0.1.zip &&
rm bonnmotion-3.0.1.zip
cd bonnmotion-3.0.1
chmod +rx *.sh *.bat ./install
./install
mkdir -p ~/.local/bin
ln -sf ~/bonnmotion-3.0.1/bin/bm ~/.local/bin
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

# NDN Intertrust Design

## Description
NDN's Intertrust proposal is a solution to allow for NDN Apps located in distinct NDN Zones (name domains) to communicate with each other. For this, they need to recognize and verify each others' trust schema. In this sense, Intertrust proposes that, for each request for content available in another NDN Zone, we include the trust schema rules of that Zone in our own trust schema file, of our Zone. For this, we need modify the schema and also sign the trust anchor of the other Zone with our trust schema. 

To the best of our knowledge, Up to this moment, no implementation of Intertrust has been performed. Therefore, we focus in implementing this solution in ndnSIM simulator.

## Intrazone communication

### 1. Assumptions
We assume that the following data has been shared among NDN entities in an out-of-band manner prior to any NDN communication:
- trust anchor (public certificate)
- trust schema (containing only the trust anchor rule)

### 2. Bootstrapping
To allow an NDN App to communicate in the intrazone network, that App needs to pass through a secure bootstrapping process that encompasses the following steps:
1) Create NDN Producer Identity/PKI (performed locally by the Producer itself)
2) Authenticate Producer (Zone Controller)
3) Sign Producer certificate with trust anchor, and send it back to Producer (Zone Controller)
4) Send updated Trust Schema to interested parties, both Consumers and Producers (Zone Controller)

These steps are described in further details in the following sections.

#### 2.1. Producer PKI Creation
When producer starts, it creates its PKI (private and public keys), as well as its public certificate. Then, it tries to authenticate with the Zone Controller. 

####  2.2. Producer Authentication
1) **Auth (Interest)**: The producers requests for authentication.
   - **Packet**: INTEREST
   - **Name**: ``/<zone>/AUTH/<producer_identity>``
2) **Challenge (Interest)**: Zone Controller requests a challenge response.
   - **Packet**: INTEREST
   - **Name**: ``/<zone>/CHG/<producer_identity>``
3) **Challenge (Data)**: The producer answers the challenge.
   - **Packet**: DATA
   - **Name**: ``/<zone>/CHG/<producer_identity>``
4) **Auth (Data)**: If the answer is correct, the Zone Controller sends an `ACK` reply. If not, it answers with `NACK`. 
   - **Packet**: DATA
   - **Name**: ``/<zone>/AUTH/<producer_identity>``
   - In case of a ``NACK`` reply, the Zone Controller could also choose not send the Data packet. That is, it will wait for the authentication protocol to timeout. This is to avoid potential DoS attacks.
   
####  2.3. Producer Certificate Signing
1) **Sign (Interest)**: The producers requests for certificate signing.
   - **Packet**: INTEREST
   - **Name**: ``/<zone>/SIGN/<producer_identity>/KEY/<>``
2) **KEY (Interest)**: Zone Controller requests the producer's certificate.
   - **Packet**: INTEREST
   - **Name**: ``/<producer_identity>/KEY/<>?canBePrefix``
3) **KEY (Data)**: The producer answers with its self-signed certificate.
   - **Packet**: DATA
   - **Name**: ``/<producer_identity>/KEY/<>{3,3}``
4) **Sign (Data)**: 
   1) The Zone Controller signs the certificate with the Zone's trust anchor.
   2) The Zone Controller sends the signed certificate back to the Producer, so that it can serve this certificate to Consumers, as they request it.
      - **Packet**: DATA
      - **Name**: ``/<zone>/SIGN/<producer_identity>/KEY/<>``

####  2.4. Update Trust Schema
1) The Zone Controller adds the Producer signed certificate to the trust schema validation rules.
2) The Zone Controller issues an update notification to interested parties (Consumers and Producers). We assume that interested parties have previously issued a subscribe Interest (``/<zone>/SCHEMA/SUBSCRIBE/<zone>``) for the trust schema.
   - **Packet**: DATA
   - **Name**: ``/<zone>/SCHEMA/SUBSCRIBE/<zone>``
3) Interested parties request the updated trust schema from Zone Controller
   - **Packet**: INTEREST
   - **Name**: ``/<zone>/SCHEMA/CONTENT/<zone>``
4) Zone Controller replies with the updated trust schema
   - **Packet**: DATA
   - **Name**: ``/<zone>/SCHEMA/CONTENT/<zone>``

### 3. Bootstrapping Overview
These following sequence diagram summarizes the bootstrapping process:

```mermaid
sequenceDiagram
    participant Producer
    participant Zone Controller

    Producer->>Zone Controller: I1: /<zone>/SCHEMA/SUBSCRIBE/<zone>    
    Producer->>Producer: createIdentityPKI()
    Producer->>Zone Controller: I2: /<zone>/AUTH/<producer_identity>
    Zone Controller->>Producer: I3: /<zone>/CHG/<producer_identity>
    Producer->>Zone Controller: D3: /<zone>/CHG/<producer_identity>
    Zone Controller->>Producer: D2: /<zone>/AUTH/<producer_identity>
    Producer->>Zone Controller: I4: /<zone>/SIGN/<producer_identity>/KEY/<>?canBePrefix
    Zone Controller->>Producer: I5: /<producer_identity>/KEY/<>?canBePrefix
    Producer->>Zone Controller: D5: /<producer_identity>/KEY/<>{3,3}
    Zone Controller->>Zone Controller: signCertWithTrustAnchor()
    Zone Controller->>Zone Controller: addSignedCertTrustSchema()
    Zone Controller->>Producer: D4: /<zone>/SIGN/<producer_identity>/KEY/<>{3,3}
    Zone Controller->>Producer: D1: /<zone>/SCHEMA/SUBSCRIBE/<zone>    
    Producer->>Zone Controller: I6: /<zone>/SCHEMA/CONTENT/<zone>
    Zone Controller->>Producer: D6: /<zone>/SCHEMA/CONTENT/<zone>    
```

## Interzone communication

### 1. Assumptions
No additional assumptions, other than the intrazone ones.

### 2. Bootstrapping
To allow an NDN App of a Zone A to consume data produced in an external Zone B (interzone communication), the Zone a controller need to:
1) Acquire Zone B self-signed trust anchor (``Tb``)
2) Sign Zone B trust anchor, creating ``Tb'``, which acts as a ``Proof of Zone Recognition``
3) Acquire Zone B validation rules (external trust schema)
4) Adapt validation rules such that KeyLocator certificate chain terminates in ``Ta`` (trust anchor of Zone A) via ``Tb'`` 
5) Include adapted Zone B rules in trust schema
6) Send updated Trust Schema to interested parties, both Consumers and Producers

These steps are described in further details in the following sections.

####  2.1. Acquire Zone B self-signed trust anchor (``Tb``)
1) **KEY (Interest)**: The Zone A controller requests Zone B trust anchor.
   - **Packet**: INTEREST
   - **Name**: ``/<zoneB>/KEY/<>{3,3}``
2) **KEY (Data)**: Zone B sends trust anchor self-signed certificate. 
   - **Packet**: DATA
   - **Name**: ``/<zoneB>/KEY/<>{3,3}``

####  2.2. Sign Zone B trust anchor
1) Zone A signs ``Tb`` to create ``Tb'`` (Proof of Zone Recognition - PZR). 
   - **Tb'** = ``/<zoneA>/<zoneB>/KEY/<>{3,3}``
2) Add rule to point ``Tb'`` as a certificate signed by ``Ta`` (Zone A trust anchor)
   - **Certificate Name**: ``/<zoneA>/<zoneB>/KEY/<>{3,3}``
   - **KeyLocator**: ``/<zoneA>/KEY/<>{3,3}``

``Tb'`` will be used to replace Zone B validation rules that terminated in ``Tb``, such that Data packet validation rules always terminate in ``Ta`` (trust anchor of Zone A).

####  2.3. Acquire Zone B validation rules
1) **SCHEMA (Interest)**: The Zone A controller requests Zone B trust schema.
   - **Packet**: INTEREST
   - **Name**: ``/<zoneB>/SCHEMA/CONTENT/<zoneA>``
2) **SCHEMA (Data)**: Zone B sends external trust schema, according to what Data packets it wants Zone A entities to consume (similar to Access Control List in IP). Zone B can also send its trust schema in its entirety, if no ACL control is needed.
   - **Packet**: DATA
   - **Name**: ``/<zoneB>/SCHEMA/CONTENT/<zoneA>``


####  2.4. Adapt validation rules 

Change ``KeyLocator`` in the received trust schema rules that have ``Tb`` as trust anchor to `Tb'`. 

####  2.5. Include adapted Zone B rules in trust schema

Include adapted rules inside Zone A trust schema. 

####  2.5. Send updated Zone A trust schema to interested parties

1) The Zone Controller issues an update notification to interested parties (Consumers and Producers). We assume that interested parties have previously issued a subscribe Interest (``/<zoneA>/SCHEMA/<zoneB>/SUBSCRIBE``) for the trust schema.
   - **Packet**: DATA
   - **Name**: ``/<zoneA>/SCHEMA/SUBSCRIBE/<zoneA>/``
2) Interested parties request the updated trust schema from Zone Controller
   - **Packet**: INTEREST
   - **Name**: ``/<zoneA>/SCHEMA/CONTENT/<zoneA>``
3) Zone Controller replies with the updated trust schema
   - **Packet**: DATA
   - **Name**: ``/<zoneA>/SCHEMA/CONTENT/<zoneA>``

### 3. Bootstrapping Overview
These following sequence diagram summarizes the bootstrapping process:

```mermaid
sequenceDiagram
    participant Zone A entities
    participant Zone A
    participant Zone B

    
    Zone A entities->>Zone A: I1: /<zoneA>/SCHEMA/SUBSCRIBE/<zoneA>
    Zone A->>Zone B: I2: /<zoneB>/KEY/<>{3,3}   
    Zone B->>Zone A: D2: /<zoneB>/KEY/<>{3,3}   
    Zone A->>Zone A: Tb' = signTrustAnchor(Tb)
    Zone A->>Zone A: addProofZoneRecognition(Tb')
    Zone A->>Zone B: I3: /<zoneB>/SCHEMA/CONTENT/<zoneA>
    Zone B->>Zone A: D3: /<zoneB>/SCHEMA/CONTENT/<zoneA>
    Zone A->>Zone A: rules' = adaptValidationRules(rules)
    Zone A->>Zone A: includeInTrustSchema(rules')
    Zone A->>Zone A entities: D4: /<zoneA>/SCHEMA/SUBSCRIBE/<zoneA>
    Zone A entities->>Zone A: I4: /<zoneA>/SCHEMA/CONTENT/<zoneA>
    Zone A->>Zone A entities: D4: /<zoneA>/SCHEMA/CONTENT/<zoneA>
```

### 4. Example (study case)
Given the following scenario:
- **Zone A**: ``/digifort``
- **Zone B**: ``/civilpolice``
    
Suppose that:
- All packets are signed directly by the trust anchor of `/civilpolice`.
- All packets follow the naming schema below:

**Naming schema**:
  - **Packet names**: `/civilpolice/<sensor>/<address>/<id>`
    - **E.g.**: `/civilpolice/camera/elm_street/001`
  - **KeyLocators**: `/civilpolice/KEY/01/self/v=01`

Suppose that `/digifort` applications need to acquire images from a given camera, to identify possible thieves or malicious actors in the city. In that case, ``/digifort`` needs to acquire validation rules of `/civilpolice` zone, as illustrated below:

```mermaid

sequenceDiagram
    participant Digifort Apps
    participant /digifort
    participant /civilpolice

    Digifort Apps->>/digifort: I1: /digifort/SCHEMA/SUBSCRIBE/digifort
    /digifort->>/civilpolice: I2: /civilpolice/KEY/01/self/v=01
    /civilpolice->>/digifort: D2: /civilpolice/KEY/01/self/v=01
    /digifort->>/digifort: /digifort/civilpolice/KEY/01/self/v=01 = signTrustAnchor(/civilpolice/KEY/01/self/v=01)
    /digifort->>/digifort: addProofZoneRecognition(/digifort/civilpolice/KEY/01/self/v=01)
    /digifort->>/civilpolice: I3: /civilpolice/SCHEMA/CONTENT/digifort
    /civilpolice->>/digifort: D3: /civilpolice/SCHEMA/CONTENT/digifort
    /digifort->>/digifort: rules' = adaptValidationRules(/civilpolice/SCHEMA/CONTENT/digifort)
    /digifort->>/digifort: includeInTrustSchema(rules')
    /digifort->>Digifort Apps: D4: /digifort/SCHEMA/SUBSCRIBE/digifort
    Digifort Apps->>/digifort: I4: /digifort/SCHEMA/CONTENT/digifort
    /digifort->>Digifort Apps: D4: /digifort/SCHEMA/CONTENT/digifort
```

```python
def addProofZoneRecognition(certificate) -> rule:
   # modify zone trust schema to include the following rule
   return """
   rule
   {
      id Proof of zone recognition validation rule
      for data
      filter
      {
         type name
         regex ^<digifort><civilpolice><KEY><>{1,3}$
      }
      checker
      {
         type customized
         sig-type rsa-sha256
         key-locator
         {
               type name
               regex "^<civilpolice><KEY><>{1,3}$"
         }
      }
   }
   """

def adaptValidationRules(external_schema) -> adapted_schema:
   # modify validation rules to point to /digifort/civilpolice KeyLocator
   return """
   rule
   {
      id Civil police adapted rule (to terminate in Tb')
      for data
      filter
      {
         type name
         regex ^<civilpolice>[^<KEY>]*$
      }
      checker
      {
         type customized
         sig-type rsa-sha256
         key-locator
         {
               type name
               regex "^<digifort><civilpolice><KEY><>{1,3}$"
         }
      }
   }
   """

def includeInTrustSchema(adapted_schema) -> None:
   # modify /digifort trust schema to include the adapted validation rules
   schema = digifort_schema.read() # read current /digifort schema
   schema.extend(adapted_schema) # add adapted rules to schema
   digifort_schema.write(schema) # save schema to disk
```