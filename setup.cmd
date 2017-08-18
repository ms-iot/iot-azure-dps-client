
echo .
echo "Using CMAKE to set up Azure projects"
echo .
echo "Disabled x64 for now as the Azure SDK is throwing few errors-following up with Azure separately"
cd deps\azure-iot-device-auth
mkdir x86
REM mkdir x64
mkdir arm
cd x86
cmake -G "Visual Studio 14 2015" .. -Ddps_auth_type=tpm
cd ..\arm
cmake -G "Visual Studio 14 2015 ARM" .. -Ddps_auth_type=tpm
REM cd ..\x64
REM cmake -G "Visual Studio 14 2015 Win64" .. -Ddps_auth_type=tpm


echo .
echo "Building Azure SDK libraries"
echo .
cd ..\x86
msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild dps_client\dps_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild dps_client\dps_http_transport.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild dps_client\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0

cd ..\arm
msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild dps_client\dps_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild dps_client\dps_http_transport.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild dps_client\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0

REM cd ..\x64
REM msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
REM msbuild dps_client\dps_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
REM msbuild dps_client\dps_http_transport.vcxproj /p:TargetPlatformVersion=10.0.14393.0
REM msbuild dps_client\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0


echo .
echo "Building IotCoreDpsClient"
echo .
cd ..\..\..
msbuild IotCoreDpsClient\IotCoreDpsClient.vcxproj /p:Platform=Win32
msbuild IotCoreDpsClient\IotCoreDpsClient.vcxproj /p:Platform=ARM
REM msbuild IotCoreDpsClient\IotCoreDpsClient.vcxproj /p:Platform=X64

