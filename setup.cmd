
echo .
echo "Using CMAKE to set up Azure projects"
echo .
cd deps\azure-iot-sdk-c
mkdir x86
mkdir arm
mkdir x64

cd x86
cmake -G "Visual Studio 15 2017" .. -Duse_prov_client:BOOL=ON -Duse_tpm_simulator:BOOL=OFF ..
cd ..\arm
cmake -G "Visual Studio 15 2017 ARM" .. -Duse_prov_client:BOOL=ON -Duse_tpm_simulator:BOOL=OFF ..
cd ..\x64
cmake -G "Visual Studio 15 2017 Win64" .. -Duse_prov_client:BOOL=ON -Duse_tpm_simulator:BOOL=OFF ..

echo .
echo "Building Azure SDK libraries"
echo .
cd ..\x86
msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\prov_device_ll_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\prov_http_transport.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0



cd ..\arm
msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\prov_device_ll_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\prov_http_transport.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0


cd ..\x64
msbuild c-utility\aziotsharedutil.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\prov_device_ll_client.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\prov_http_transport.vcxproj /p:TargetPlatformVersion=10.0.14393.0
msbuild provisioning_client\tools\tpm_device_provision\tpm_device_provision.vcxproj /p:TargetPlatformVersion=10.0.14393.0

echo .
echo "Building IotDpsClient"
echo .
cd ..\..\..
msbuild src\IotDpsClient\IotDpsClient.vcxproj /p:Platform=Win32
msbuild src\IotDpsClient\IotDpsClient.vcxproj /p:Platform=ARM
msbuild src\IotDpsClient\IotDpsClient.vcxproj /p:Platform=x64