# iot-core-azure-dps-client

## get from github
Clone recursively:

    git clone --recursive https://github.com/ms-iot/iot-core-azure-dps-client

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
Deploy the provisioning tool (tpm_device_provision.exe built in the previous step) to IotCore device.

Get ENDORSEMENT_KEY from TPM on IotCore device using limpet tool:

    limpet -erk

Run provisioning tool (get CONNECTION_STRING from Azure DPS service, DEVICE_NAME is up to you):

    tpm_device_provision.exe -c <CONNECTION_STRING> -d <DEVICE_NAME> -e <ENDORSEMENT_KEY>

## configure registry for IotCoreDpsClient
Add this information to the registry:
* `TPM_SLOT`: logical slot in TPM to store iothub connection string
* `DPS_URI`: URI of DPS service containing information for this device
* `DPS_SCOPE`: TBD (get this from Azure)

To do this from a command line, run commands like this:

    reg add HKLM\System\CurrentControlSet\Control\Wininit /v dps_slot /t REG_SZ /d <TPM_SLOT>
    reg add HKLM\System\CurrentControlSet\Control\Wininit /v dps_uri /t REG_SZ /d <DPS_URI>
    reg add HKLM\System\CurrentControlSet\Control\Wininit /v dps_scope /t REG_SZ /d <DPS_SCOPE>


## configure IotCoreDpsClient
Deploy IotCoreDpsClient.exe to IotCore device.

Configure as service:

    IotCoreDpsClient.exe -install
    sc config IotCoreDpsClient start=auto
    sc config IotCoreDpsClient depend=w32time

Run from command line:

    IotCoreDpsClient.exe -debug

