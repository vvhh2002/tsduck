﻿#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Download and install NSIS, the NullSoft Installer Scripting.

 .PARAMETER ForceDownload

  Force a download even if NSIS is already downloaded.

 .PARAMETER NoInstall

  Do not install the NSIS package. By default, NSIS is installed.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [switch]$ForceDownload = $false,
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false
)

Write-Output "NSIS download and installation procedure"
$NSISPage = "https://nsis.sourceforge.io/Download"
$FallbackURL = "http://prdownloads.sourceforge.net/nsis/nsis-3.05-setup.exe?download"

# A function to exit this script.
function Exit-Script([string]$Message = "")
{
    $Code = 0
    if ($Message -ne "") {
        Write-Host "ERROR: $Message"
        $Code = 1
    }
    if (-not $NoPause) {
        pause
    }
    exit $Code
}

# Local file names.
$RootDir = $PSScriptRoot
$ExtDir = "$RootDir\msvc\tmp"

# Create the directory for external products when necessary.
[void] (New-Item -Path $ExtDir -ItemType Directory -Force)

# Without this, Invoke-WebRequest is awfully slow.
$ProgressPreference = 'SilentlyContinue'

# Get the HTML page for NSIS downloads.
$status = 0
$message = ""
$Ref = $null
try {
    $response = Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $NSISPage
    $status = [int] [Math]::Floor($response.StatusCode / 100)
}
catch {
    $message = $_.Exception.Message
}

if ($status -ne 1 -and $status -ne 2) {
    # Error fetch NSIS download page.
    if ($message -eq "" -and (Test-Path variable:response)) {
        Write-Output "Status code $($response.StatusCode), $($response.StatusDescription)"
    }
    else {
        Write-Output "#### Error accessing ${NSISPage}: $message"
    }
}
else {
    # Parse HTML page to locate the latest installer.
    $Ref = $response.Links.href | Where-Object { $_ -like "*/nsis-*-setup.exe?download" } | Select-Object -First 1
}

if (-not $Ref) {
    # Could not find a reference to NSIS installer.
    $Url = [System.Uri]$FallbackURL
}
else {
    # Build the absolute URL's from base URL (the download page) and href links.
    $Url = New-Object -TypeName 'System.Uri' -ArgumentList ([System.Uri]$NSISPage, $Ref)
}

$InstallerName = (Split-Path -Leaf $Url.LocalPath)
$InstallerPath = "$ExtDir\$InstallerName"

# Download installer
if (-not $ForceDownload -and (Test-Path $InstallerPath)) {
    Write-Output "$InstallerName already downloaded, use -ForceDownload to download again"
}
else {
    Write-Output "Downloading $Url ..."
    Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $Url -OutFile $InstallerPath
    if (-not (Test-Path $InstallerPath)) {
        Exit-Script "$Url download failed"
    }
}

# Install NSIS
if (-not $NoInstall) {
    Write-Output "Installing $InstallerName"
    Start-Process -FilePath $InstallerPath -ArgumentList @("/S") -Wait
}

Exit-Script
