param 
(
    [string] $ReadMeDir,
    [string] $HelpDir,
    [string] $TargetDir,
    [switch] $Rebuild
)

Set-StrictMode -Version Latest

Set-Variable -Name HH_CMD -Value "C:\Program Files (x86)\HTML Help Workshop\hhc.exe" -Option Constant 
Set-Variable -Name CHM_FILE -Value "ClassicTileCascadeHelp.chm" -Option Constant
Set-Variable -Name HHP_FILE -Value "ClassicTileCascadeHelp.hhp" -Option Constant
Set-Variable -Name MD_FILE -Value "README.md" -Option Constant
Set-Variable -Name MD_INTERIM_DIR -Value "MD_Files" -Option Constant
Set-Variable -Name LUA_FILTER -Value "image-filter.lua" -Option Constant

[string] $ChmPath = Join-Path -Path "$HelpDir" -ChildPath $script:CHM_FILE
[string] $HhpPath = Join-Path -Path "$HelpDir" -ChildPath $script:HHP_FILE
[string] $TargetChmPath = Join-Path -Path "$TargetDir" -ChildPath $script:CHM_FILE


[Nullable[datetime]] $LatestDependency =  Get-ChildItem -Path "$HelpDir\*" -Include *.htm, *.hhc, *.hhp, *.png | Measure-Object -Property LastWriteTime -Maximum | foreach {$_.Maximum}
[Nullable[datetime]] $ChmFileTime = Get-ChildItem -Path $HelpDir -File $script:CHM_FILE  | foreach {$_.LastWriteTime}
[Nullable[datetime]] $TargetChmFileTime = Get-ChildItem -Path $TargetDir -File $script:CHM_FILE  | foreach {$_.LastWriteTime}


if($Rebuild -or ($LatestDependency -gt $ChmFileTime)){
    "Starting CHM build"
    "Compiling HTML Help Project '$HhpPath'"
    & $script:HH_CMD "`"$HhpPath`""
    "Copying '$ChmPath' to '$TargetChmPath'"
    Copy-Item -path $ChmPath -Destination $TargetChmPath 
 }elseif($ChmFileTime -gt $TargetChmFileTime){
    "Starting CHM build"
    "Copying '$ChmPath' to '$TargetChmPath'"
     Copy-Item -path $ChmPath -Destination $TargetChmPath 
 }else{
    "CHM is up to date"
 }

""
[string] $TargetReadMePath = Join-Path -Path "$ReadMeDir" -ChildPath $script:MD_FILE
[string] $InterimReadMeDir = Join-Path -Path "$HelpDir" -ChildPath $script:MD_INTERIM_DIR
[string] $InterimReadMePath = Join-Path -Path $InterimReadMeDir -ChildPath $script:MD_FILE
[string] $LuaFilterPath =Join-Path -Path $HelpDir -ChildPath $script:LUA_FILTER

[Nullable[datetime]] $LatestReadmeDependency =  Get-ChildItem -Path "$HelpDir\*" -Include *.htm, *.md | Measure-Object -Property LastWriteTime -Maximum | foreach {$_.Maximum}
[Nullable[datetime]] $InterimReadmeFileTime = Get-ChildItem -Path $InterimReadMeDir -File $script:MD_FILE  | foreach {$_.LastWriteTime}
[Nullable[datetime]] $TargetReadmeFileTime = Get-ChildItem -Path $ReadMeDir -File $script:MD_FILE  | foreach {$_.LastWriteTime}


 if($Rebuild -or ($LatestReadmeDependency -gt $InterimReadmeFileTime)){
    "Staring README.md build"
    Remove-Item -path "$InterimReadMeDir\*" -Include *.md
    Get-ChildItem -Path "$HelpDir\*" -Include *.htm | foreach {
        [System.IO.FileInfo] $item = Get-Item $_
        [string] $MDName = Join-Path -path $InterimReadMeDir -ChildPath "$($_.BaseName).md"
        "Converting '$_' to '$MDName'"
        & pandoc -f html -t gfm -s --wrap=auto --lua-filter="`"$LuaFilterPath`"" -o "`"$MDName`"" "`"$_`""
        Add-Content -LiteralPath $MDName -Value "`r`n" 
    }

    Get-ChildItem -Path "$HelpDir\*" -Include *.md | foreach {
        "Copying '$_' to '$InterimReadMeDir'"
        Copy-Item -path $_.FullName -Destination $InterimReadMeDir
    }
    
    [string[]] $md_files = @(
                            'Welcome.md'
	                        'Dependencies.md'
	                        'Starting Classic Tile.md'
	                        'Usage.md'
	                        'Basic Operation.md'
	                        'Settings Menu.md'
	                        'Troubleshooting.md'
	                        'Other.md'
	                        'License.md'
	                        'Credits.md'
                            )
    
    [string[]] $md_file_list_arr = @()
    "Combining..."
    $md_files | foreach{
        [string] $thisPath = Join-Path -path $InterimReadMeDir -childpath $_

        "`t`"$thisPath`""
        
        $md_file_list_arr += $thisPath
    }

    "...to '$InterimReadMePath'"

    Get-Content -LiteralPath $md_file_list_arr | Set-Content $InterimReadMePath

    "Copying '$InterimReadMePath' to '$TargetReadMePath'"
    Copy-Item -path $InterimReadMePath -Destination $TargetReadMePath 
    
 }elseif($InterimReadmeFileTime -gt $TargetReadmeFileTime){
    "Staring README.md build"
    "Copying '$InterimReadMePath' to '$TargetReadMePath'"
    Copy-Item -path $InterimReadMePath -Destination $TargetReadMePath 
 }else{
    "Readme.MD is up to date"
 }