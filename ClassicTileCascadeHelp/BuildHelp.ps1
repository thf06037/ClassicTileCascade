param 
(
    [string] $HelpDir,
    [string] $TargetDir,
    [switch] $Rebuild
)

Set-StrictMode -Version Latest

Set-Variable -Name HH_CMD -Value "C:\Program Files (x86)\HTML Help Workshop\hhc.exe" -Option Constant 
Set-Variable -Name CHM_FILE -Value "ClassicTileCascadeHelp.chm" -Option Constant
Set-Variable -Name HHP_FILE -Value "ClassicTileCascadeHelp.hhp" -Option Constant

[string] $ChmPath = Join-Path -Path "$HelpDir" -ChildPath $script:CHM_FILE
[string] $HhpPath = Join-Path -Path "$HelpDir" -ChildPath $script:HHP_FILE
[string] $TargetChmPath = Join-Path -Path "$TargetDir" -ChildPath $script:CHM_FILE
[Nullable[datetime]] $LatestDependency =  Get-ChildItem -Path "$HelpDir\*" -Include *.htm, *.hhc, *.hhp, *.png | Measure-Object -Property LastWriteTime -Maximum | foreach {$_.Maximum}
[Nullable[datetime]] $ChmFileTime = Get-ChildItem -Path $HelpDir -File $script:CHM_FILE  | foreach {$_.LastWriteTime}
[Nullable[datetime]] $TargetChmFileTime = Get-ChildItem -Path $TargetDir -File $script:CHM_FILE  | foreach {$_.LastWriteTime}

if($Rebuild -or ($LatestDependency -gt $ChmFileTime)){
    "Compiling HTML Help Project '$HhpPath'"
    & $script:HH_CMD "`"$HhpPath`""
    "Copying '$ChmPath' to '$TargetChmPath'"
    Copy-Item -path $ChmPath -Destination $TargetChmPath 
 }elseif($ChmFileTime -gt $TargetChmFileTime){
    "Copying '$ChmPath' to '$TargetChmPath'"
     Copy-Item -path $ChmPath -Destination $TargetChmPath 
 }else{
    "CHM is up to date"
 }