<#
.SYNOPSIS
  Check that Windows will actually start SpaceMonger.exe.

.DESCRIPTION
  The unit tests never load the application's embedded manifest. A merged
  manifest with a duplicate element (for example two <dpiAware> entries, one
  from SpaceMonger.manifest and one added by the manifest tool) builds and
  passes every test, but Windows refuses to start the program with "the
  side-by-side configuration is incorrect".

  This script checks the embedded manifest for duplicate settings, then
  launches the executable, requires it to still be running after a few
  seconds, and closes it.

  Based on tools/smoke-test.ps1 from JRpersonal/spacemonger (MIT).

.PARAMETER Path
  Path to SpaceMonger.exe.

.PARAMETER ManifestOnly
  Only check the manifest. Used for ARM64 builds, which cannot run on the
  x64 CI runner.
#>
param(
  [Parameter(Mandatory = $true)][string]$Path,
  [int]$Seconds = 5,
  [switch]$ManifestOnly
)
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $Path)) { throw "File not found: $Path" }
$fullPath = (Resolve-Path -LiteralPath $Path).ProviderPath

# The embedded RT_MANIFEST resource is plain UTF-8 XML inside the PE file.
# Search ordinally: under pwsh's ICU globalization NUL is ignorable, so a
# culture-sensitive IndexOf would also match UTF-16 "<assembly" text.
$text = [Text.Encoding]::UTF8.GetString([IO.File]::ReadAllBytes($fullPath))
$start = $text.IndexOf('<assembly', [StringComparison]::Ordinal)
if ($start -lt 0) { throw "No embedded application manifest found in $Path" }
$end = $text.IndexOf('</assembly>', $start, [StringComparison]::Ordinal)
if ($end -lt 0) { throw "Embedded manifest in $Path is not terminated." }
$manifest = $text.Substring($start, $end - $start + '</assembly>'.Length)

# mt.exe may write merged elements with a namespace prefix
# (e.g. <ms_windowsSettings:dpiAware>), so allow one.
foreach ($element in 'dpiAware', 'dpiAwareness', 'longPathAware', 'requestedExecutionLevel') {
  $count = ([regex]::Matches($manifest, "<(?:[\w.-]+:)?$element[\s>/]")).Count
  if ($count -gt 1) {
    throw "Embedded manifest has $count <$element> elements; Windows will refuse to start it.`n$manifest"
  }
}
Write-Host "Embedded manifest OK: $Path"
if ($ManifestOnly) { return }

try {
  $process = Start-Process -FilePath $fullPath -PassThru
} catch {
  throw "Windows could not start ${Path}: $($_.Exception.Message)"
}
Start-Sleep -Seconds $Seconds
if ($process.HasExited) { throw "$Path exited after start with code $($process.ExitCode)." }
Stop-Process -Id $process.Id -Force
Write-Host "Started and still running after $Seconds s: $Path"
