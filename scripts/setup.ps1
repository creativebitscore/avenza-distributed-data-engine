param([ValidateSet('Debug','Release')][string]$BuildType='Release',[switch]$FetchGoogleTest)
$ErrorActionPreference='Stop'
if(-not (Get-Command cmake -ErrorAction SilentlyContinue)){ throw 'CMake 3.20+ is required. Install with: winget install Kitware.CMake' }
$fetch=if($FetchGoogleTest){'ON'}else{'OFF'}
cmake -S $PSScriptRoot\.. -B $PSScriptRoot\..\build -DCMAKE_BUILD_TYPE=$BuildType -DAVENZA_FETCH_GTEST=$fetch
cmake --build $PSScriptRoot\..\build --config $BuildType --parallel 2
ctest --test-dir $PSScriptRoot\..\build -C $BuildType --output-on-failure
