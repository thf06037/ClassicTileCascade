﻿param 
(
    [string] $ReadMeDir,
    [string] $HelpDir,
    [string] $TargetDir,
    [switch] $Rebuild
)

Set-StrictMode -Version Latest

Set-Variable -Name MAX_PATH -Value 260 -Option Constant


[System.Type] $shlwapi = Add-Type -name "shlwapi" -Namespace Win32Functions -PassThru -MemberDefinition @'
    [DllImport("shlwapi.dll", CharSet = CharSet.Unicode)]
    public static extern bool PathQuoteSpaces(Char[] lpsz);
'@

function Get-PathQuoteSpace
{
    [OutputType([string])]
    Param(
        [Parameter(Mandatory=$true, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True, Position=0)]
        [String] $Path
    )

    process
    {
        [Char[]] $sbPath = [Char[]]::new($script:MAX_PATH)
        $Path.CopyTo(0, $sbPath, 0, $Path.Length)
        $shlwapi::PathQuoteSpaces($sbPath) | Out-Null

        [int] $zInd = [array]::IndexOf($sbPath, [char]0)
        if($zInd -lt 0){
            [string]::new($sbPath)
        }else{
            [string]::new($sbPath, 0, $zInd)
        }
    }
}

function Get-LatestWriteTime
{
    [OutputType([Nullable[datetime]])]
    Param(
        [Parameter(Mandatory=$true, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True, Position=0)]
        [string] $Path,

        [Parameter(Mandatory=$false, ValueFromPipelineByPropertyName=$True, Position=1)]
        [string[]] $Include
    )

    process
    {
        Get-ChildItem @PSBoundParameters | Measure-Object -Property LastWriteTime -Maximum | Select-Object -ExpandProperty Maximum
    }

}

function Get-LastWriteTime
{
    [OutputType([Nullable[datetime]])]
    Param(
        [Parameter(Mandatory=$true, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True, Position=0)]
        [string] $Path,

        [Parameter(Mandatory=$true, ValueFromPipelineByPropertyName=$True, Position=1)]
        [string] $Filter
    )

    process
    {
        Get-ChildItem @PSBoundParameters -File | Select-Object -ExpandProperty LastWriteTime
    }
}

function Join-PathAndQuote
{
    [OutputType([string])]
    Param(
        [Parameter(Mandatory=$true, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True, Position=0)]
        [string] $Path,

        [Parameter(Mandatory=$true, ValueFromPipelineByPropertyName=$True, Position=1)]
        [string] $ChildPath
    )

    Join-Path @PSBoundParameters | Get-PathQuoteSpace
}

Set-Variable -Name HH_CMD -Value "C:\Program Files (x86)\HTML Help Workshop\hhc.exe" -Option Constant 
Set-Variable -Name CHM_FILE -Value "ClassicTileCascadeHelp.chm" -Option Constant
Set-Variable -Name HHP_FILE -Value "ClassicTileCascadeHelp.hhp" -Option Constant
Set-Variable -Name MD_FILE -Value "README.md" -Option Constant
Set-Variable -Name MD_INTERIM_DIR -Value "MD_Files" -Option Constant
Set-Variable -Name LUA_FILTER -Value "image-filter.lua" -Option Constant

[string] $ChmPath = Join-Path -Path "$HelpDir" -ChildPath $script:CHM_FILE
[string] $HhpPath = Join-PathAndQuote -Path "$HelpDir" -ChildPath $script:HHP_FILE
[string] $TargetChmPath = Join-Path -Path "$TargetDir" -ChildPath $script:CHM_FILE


[Nullable[datetime]] $LatestDependency =  Get-LatestWriteTime -Path "$HelpDir\*" -Include *.htm, *.hhc, *.hhp, *.png
[Nullable[datetime]] $ChmFileTime = Get-LastWriteTime -Path $HelpDir -Filter $script:CHM_FILE  
[Nullable[datetime]] $TargetChmFileTime = Get-LastWriteTime -Path $TargetDir -Filter  $script:CHM_FILE  

if($Rebuild -or ($LatestDependency -gt $ChmFileTime)){
    "Starting CHM build"
    "Compiling HTML Help Project '$HhpPath'"
    & $script:HH_CMD $HhpPath | where {$_.trim() -ne ""}
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
[string] $LuaFilterPath =Join-PathAndQuote -Path $HelpDir -ChildPath $script:LUA_FILTER

[Nullable[datetime]] $LatestReadmeDependency =  Get-LatestWriteTime -Path "$HelpDir\*" -Include *.htm, *.md 
[Nullable[datetime]] $InterimReadmeFileTime = Get-LastWriteTime -Path $InterimReadMeDir -Filter $script:MD_FILE  
[Nullable[datetime]] $TargetReadmeFileTime = Get-LastWriteTime -Path $ReadMeDir -Filter $script:MD_FILE  

 if($Rebuild -or ($LatestReadmeDependency -gt $InterimReadmeFileTime)){
    "Starting README.md build"
    Remove-Item -path "$InterimReadMeDir\*" -Include *.md
    Get-ChildItem -Path "$HelpDir\*" -Include *.htm | foreach {
        [string] $MDName = Join-Path -path $InterimReadMeDir -ChildPath "$($_.BaseName).md"
        [string] $MDNameQuote = Get-PathQuoteSpace -path $MDName
        [string] $HTMNameQuote = PathQuoteSpace -Path $_
        "Converting '$_' to '$MDName'"
        & pandoc -f html -t gfm -s --wrap=auto --lua-filter=$LuaFilterPath -o $MDNameQuote $HTMNameQuote
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

        "`t$thisPath"
        
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