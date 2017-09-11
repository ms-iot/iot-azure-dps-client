# iot-azure-dps-client

## Get from github
Clone recursively:

    git clone --recursive https://github.com/ms-iot/iot-azure-dps-client

If you find that the deps folder is empty, try this:

    git submodule update --init --recursive

## Set up development environment
Be sure you have CMAKE configured:

* Install [CMake](https://cmake.org/download/). 
* Make sure it is in your PATH by typing cmake -version from a command prompt. CMake will be used to create Visual Studio projects to build libraries and samples. 
* Make sure your CMake version is at least 3.6.1.

Be sure you have PERL configured:

* Install [perl](https://www.perl.org/get.html). You can use either ActivePerl or Strawberry Pearl. Run the installer as administrator to avoid issues.
    
Be sure you are using Visual Studio 2015 with Visual C++ (this last bit is important!)

## Build binaries for x86 and ARM

    Start a VS 2015 developer command prompt
    cd <repo>
    setup.cmd

## Setup Azure cloud resources

Setup cloud resources by following steps mentioned in https://docs.microsoft.com/en-us/azure/iot-dps/tutorial-set-up-cloud and gather below information.

    --ID Scope - You can get from Azure portal -> Device Provisioning Services -> Overview -> ID Scope.
    --Global device end point - You can get from Azure portal -> Device Provisioning Services -> Overview -> Global device endpoint.  

## Enroll the device in DPS
* Set up Windows IoT device with TPM by using the below link if you do not have already.
    https://developer.microsoft.com/en-us/windows/iot/getstarted

* Connect to device using PowerShell by using device administrator credentials from your development machine.

* Copy the provisioning tool (tpm_device_provision.exe built in the previous step) to Windows IoT device.

* Run provisioning tool from remote powershell connection.
    tpm_device_provision.exe
    Tool prints endorsement key and registration id, please note down.
    
* Enroll the device in DPS by following steps,
    https://docs.microsoft.com/en-us/azure/iot-dps/tutorial-provision-device-to-hub#enrolldevice

## Configure registry for IotDpsClient
Add this information to the registry:
* `TPM_SLOT`:   Logical slot in TPM to store the secrets. Valid values are from 0 to 9. Note down this, it will be needed for IOT DM client configuration.
* `DPS_URI`:    URI of DPS global device end point (Gathered during setup cloud resources) containing enrollment information of this device.
* `DPS_SCOPE`:  ID scope (Gathered during setup cloud resources) is assigned to a customer when they create the DPS in Azure portal.

To do this run below from remote PowerShell connection:
* reg add hklm\system\currentcontrolset\services\iotdpsclient\parameters  /v tpm_slot /t REG_SZ /d <TPM_SLOT>
* reg add hklm\system\currentcontrolset\services\iotdpsclient\parameters /v dps_uri /t REG_SZ /d <DPS_URI>
* reg add hklm\system\currentcontrolset\services\iotdpsclient\parameters /v dps_scope /t REG_SZ /d <DPS_SCOPE>

## Configure IotDpsClient
Copy the IotDpsClient.exe (IotDpsClient.exe built in the previous step) to Windows IoT device by using remote PowerShell connection.

Follow one of the below option to register the device in IotHub using DPS client,

    Option# Run the console application from remote Powershell connection.
        IotDpsClient.exe -debug

    Option# Configure as service by running the below from remote PowerShell connection.
        IotDpsClient.exe -install
        cmd /c "sc config IotDpsClient start=auto"

## Verification

* Option#1 Run from command line, limpet <TPM_SLOT> -rur
    It is expected to display service uri of the device, that confirms the device registered successfully in Azure IoT Hub.

* Option#2 You can also use the Azure portal -> iothub device explorer and can find the device.

* Option#3 Use the DM client for managing the device.

##########################################################################

## Setting up remote PowerShell connection,
	Start the PowerShell by running as administrator in your development machine.
	$ip = "<Ip Address>"
	$password = "<administrator password>"
	$username = "administrator"
	$secstr = New-Object -TypeName System.Security.SecureString
	$password.ToCharArray() | ForEach-Object {$secstr.AppendChar($_)}
	$cred = new-object -typename System.Management.Automation.PSCredential -argumentlist $username, $secstr
	Set-Item -Path WSMan:\localhost\Client\TrustedHosts -Value "$ip" -Force
	$session = New-PSSession -ComputerName $ip -Credential $cred
	Enter-Pssession $session
