# iot-azure-dps-client

## get from github
Clone recursively:

    git clone --recursive https://github.com/ms-iot/iot-azure-dps-client

If you find that the deps folder is empty, try this:

    git submodule update --init --recursive

## set up development environment
Be sure you have CMAKE configured:

* Install [CMake](https://cmake.org/download/). 
* Make sure it is in your PATH by typing cmake -version from a command prompt. CMake will be used to create Visual Studio projects to build libraries and samples. 
* Make sure your CMake version is at least 3.6.1.

Be sure you have PERL configured:

* Install [perl](https://www.perl.org/get.html). You can use either ActivePerl or Strawberry Pearl. Run the installer as administrator to avoid issues.
    
Be sure you are using Visual Studio 2015 with Visual C++ (this last bit is important!)

## build binaries for x86 and ARM

    Start a VS 2015 developer command prompt
    cd <repo>
    setup.cmd

## provision DPS
Deploy the provisioning tool (tpm_device_provision.exe built in the previous step) to Windows Iot device.

Run provisioning tool (get CONNECTION_STRING from Azure DPS service, DEVICE_NAME is up to you):

    tpm_device_provision.exe -c <CONNECTION_STRING> -d <DEVICE_NAME>

## configure registry for IotDpsClient
Add this information to the registry:
* `TPM_SLOT`: logical slot in TPM to store iothub connection string
* `DPS_URI`: URI of DPS service containing information for this device
* `DPS_SCOPE`: Scope Id is assigned to a customer when they create the DPS hub and it acts as a namespace for the registration id.

To do this from a command line, run commands like this:

    reg add hklm\system\currentcontrolset\services\iotdpsclient\parameters  /v tpm_slot /t REG_SZ /d <TPM_SLOT>
    reg add hklm\system\currentcontrolset\services\iotdpsclient\parameters /v dps_uri /t REG_SZ /d <DPS_URI>
    reg add hklm\system\currentcontrolset\services\iotdpsclient\parameters /v dps_scope /t REG_SZ /d <DPS_SCOPE>

## configure IotDpsClient
Deploy IotDpsClient.exe to Windows Iot device.

Configure as service:

    IotDpsClient.exe -install
    sc config IotDpsClient start=auto

Run from command line:

    IotDpsClient.exe -debug
