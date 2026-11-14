param(
   [Parameter(Mandatory = $true)][string]$SkiaSource,
   [ValidateSet('Release', 'Debug', 'Both')][string]$Configuration = 'Both',
   [int]$Jobs = 8
)
$ErrorActionPreference = 'Stop'
$sProject = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$sSource = (Resolve-Path -LiteralPath $SkiaSource).Path
$sPython = (Get-Command python.exe -ErrorAction Stop).Source
$sNinja = (Get-Command ninja.exe -ErrorAction Stop).Source
$sRevision = & git -C $sSource rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $sRevision -ne 'e8ff7a4187ecd9c98df14e7a617dd4ac5d74295a') {
   throw 'The source checkout does not match the pinned Skia revision.'
}
foreach ($dependency in @(
   @('freetype', '264b5fbf5b912b39f98d038bf75d39be0a73f21b'),
   @('libpng', 'd5515b5b8be3901aac04e5bd8bd5c89f287bcd33'),
   @('zlib', '646b7f569718921d7d4b5b8e22572ff6c76f2596')
)) {
   $sDependencyRevision = & git -C "$sSource/third_party/externals/$($dependency[0])" rev-parse HEAD
   if ($LASTEXITCODE -ne 0 -or $sDependencyRevision -ne $dependency[1]) {
      throw "Dependency does not match pinned revision: $($dependency[0])"
   }
}
$aConfigurations = if ($Configuration -eq 'Both') { @('Release', 'Debug') } else { @($Configuration) }
Push-Location $sSource
try {
   foreach ($sConfig in $aConfigurations) {
      $sOutput = "out/mathpad-x64-$sConfig"
      New-Item -ItemType Directory -Force $sOutput | Out-Null
      Copy-Item -LiteralPath "$sProject/Skia/Build/$sConfig.args.gn" -Destination "$sOutput/args.gn"
      & bin/gn.exe gen $sOutput "--script-executable=$sPython" --fail-on-unused-args
      if ($LASTEXITCODE -ne 0) { throw 'GN generation failed.' }
      & $sNinja -C $sOutput -j $Jobs skia
      if ($LASTEXITCODE -ne 0) { throw 'Skia compilation failed.' }
   }
} finally { Pop-Location }
& $sPython "$PSScriptRoot/Package.py" $sSource --configuration $Configuration
if ($LASTEXITCODE -ne 0) { throw 'Skia packaging failed.' }
