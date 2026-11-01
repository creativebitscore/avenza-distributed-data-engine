$ErrorActionPreference='Stop'; $root=Resolve-Path "$PSScriptRoot\.."; $exe=Join-Path $root 'build\Release\avenza-server.exe'
if(-not (Test-Path $exe)){$exe=Join-Path $root 'build\avenza-server.exe'}
if(-not (Test-Path $exe)){throw 'Build the project first.'}
1..3 | ForEach-Object { $port=7000+$_; Start-Process -WindowStyle Hidden -FilePath $exe -ArgumentList '--port',$port,'--capacity','10000','--data-dir',(Join-Path $root "data\node$_") -PassThru | Select-Object Id,@{n='Port';e={$port}} }
