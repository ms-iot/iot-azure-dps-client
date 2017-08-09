
echo .
echo "Using CMAKE to set up Azure projects"
echo .
cd deps\azure-iot-device-auth
mkdir x86
mkdir x64
mkdir arm
cd x86
cmake -G "Visual Studio 14 2015" .. -Duse_emulator=OFF -Duse_tpm_module=ON
cd ..\arm
cmake -G "Visual Studio 14 2015 ARM" .. -Duse_emulator=OFF -Duse_tpm_module=ON
cd ..\x64
cmake -G "Visual Studio 14 2015 Win64" .. -Duse_emulator=OFF -Duse_tpm_module=ON


echo .
echo "Building Azure SDK libraries"
echo .
cd ..\x86
msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\iothub_drs_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\dps_security_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0

cd ..\arm
msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\iothub_drs_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\dps_security_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0

cd ..\x64
msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\iothub_drs_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\dps_security_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild iothub_device_auth\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0


echo .
echo "Building IotCoreDpsClient"
echo .
cd ..\..\..
msbuild IotCoreDpsClient\IotCoreDpsClient.vcxproj /p:Platform=Win32 
msbuild IotCoreDpsClient\IotCoreDpsClient.vcxproj /p:Platform=ARM 
msbuild IotCoreDpsClient\IotCoreDpsClient.vcxproj /p:Platform=X64

