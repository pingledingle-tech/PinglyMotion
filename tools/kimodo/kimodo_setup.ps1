# PinglyMotion helper for NVIDIA Kimodo (text-to-motion): Windows setup and start.
# Started by PinglyMotion-Kimodo.bat. Everything goes into this helper's own
# folder, %LOCALAPPDATA%\PinglyMotion\kimodo; nothing is installed for the whole
# computer and no administrator rights are needed. Delete that folder to remove it.
#
# 1. Downloads the "uv" installer (a pinned version from PyPI, checked by its SHA-256).
# 2. Lets uv make a private Python 3.11 in the helper folder.
# 3. Hands over to kimodo_helper.py (check, licence and token, install, make).
# See tools/kimodo/README.md.

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$UvVersion = '0.13.9'
$UvWheel = 'https://files.pythonhosted.org/packages/dd/ef/ceeabf3631117f7354787d9fb1ffe943899e59ed4b887a696ed547e47e76/uv-0.13.9-py3-none-win_amd64.whl'
$UvSha256 = '6a44b2ed8111d2f60d2b92627c5dccda3f7ed7570b18ce115e569cd68c28246b'
$PythonVersion = '3.11'

if ($env:PINGLYMOTION_KIMODO_HOME) { $HelperHome = $env:PINGLYMOTION_KIMODO_HOME }
else { $HelperHome = Join-Path $env:LOCALAPPDATA 'PinglyMotion\kimodo' }
New-Item -ItemType Directory -Force -Path $HelperHome | Out-Null

$Uv = Join-Path $HelperHome "uv-$UvVersion\uv.exe"
$Venv = Join-Path $HelperHome 'venv'
$Python = Join-Path $Venv 'Scripts\python.exe'
$env:UV_CACHE_DIR = Join-Path $HelperHome 'uv-cache'
$env:UV_PYTHON_INSTALL_DIR = Join-Path $HelperHome 'python'
$env:UV_NO_CONFIG = '1'

if (-not [Environment]::Is64BitOperatingSystem) {
  Write-Host 'This helper needs 64-bit Windows.'
  exit 1
}

if (-not (Test-Path $Uv)) {
  Write-Host "Downloading the installer (uv $UvVersion, about 16 MB)..."
  $wheel = Join-Path $HelperHome 'uv.whl'
  try { Invoke-WebRequest -Uri $UvWheel -OutFile $wheel -UseBasicParsing }
  catch { Write-Host 'The download failed. Check the internet connection, then try again.'; exit 1 }
  $hash = (Get-FileHash -Algorithm SHA256 -Path $wheel).Hash.ToLowerInvariant()
  if ($hash -ne $UvSha256) {
    Remove-Item -Force $wheel
    Write-Host 'The downloaded installer is not the expected file (its checksum differs). Nothing was installed.'
    exit 1
  }
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $zip = [System.IO.Compression.ZipFile]::OpenRead($wheel)
  try {
    $entry = $zip.Entries | Where-Object { $_.FullName -eq "uv-$UvVersion.data/scripts/uv.exe" }
    if (-not $entry) { throw 'uv.exe not found in the download' }
    New-Item -ItemType Directory -Force -Path (Split-Path $Uv) | Out-Null
    [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $Uv, $true)
  } finally { $zip.Dispose() }
  Remove-Item -Force $wheel
}

if (-not (Test-Path $Python)) {
  Write-Host "Making a private Python $PythonVersion for the helper..."
  & $Uv venv --python $PythonVersion $Venv
  if ($LASTEXITCODE -ne 0) { Write-Host 'Python could not be set up. Check the internet connection, then try again.'; exit 1 }
}

$env:PINGLYMOTION_KIMODO_UV = $Uv
$env:PINGLYMOTION_KIMODO_HOME = $HelperHome
& $Python (Join-Path $PSScriptRoot 'kimodo_helper.py') @args
exit $LASTEXITCODE
