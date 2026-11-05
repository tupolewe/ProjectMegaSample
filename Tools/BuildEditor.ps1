param(
    [string]$EngineRoot = 'D:\Pliki programów (x86)\UE_5.8',
    [switch]$GenerateProjectFiles
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path.TrimEnd('\')
$EngineRoot = (Resolve-Path $EngineRoot).Path.TrimEnd('\')

# MSVC fails to write PCH files under the original non-ASCII project path.
# Keep these session-local aliases available for generated IDE projects too.
function Ensure-DriveAlias([string]$Drive, [string]$Target) {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new('subst.exe')
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.CreateNoWindow = $true
    $startInfo.StandardOutputEncoding = [System.Text.Encoding]::GetEncoding((Get-Culture).TextInfo.OEMCodePage)
    $process = [System.Diagnostics.Process]::Start($startInfo)
    $mappings = $process.StandardOutput.ReadToEnd()
    $process.WaitForExit()
    $process.Dispose()
    $mapping = $mappings -split '\r?\n' | Where-Object { $_.StartsWith("${Drive}:\:", [StringComparison]::OrdinalIgnoreCase) }
    if ($mapping) {
        $existingTarget = ($mapping -split ' => ', 2)[1].TrimEnd('\')
        if (-not [string]::Equals($existingTarget, $Target, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Drive ${Drive}: already maps to another directory: $existingTarget"
        }
        return
    }
    if (Test-Path "${Drive}:\") { throw "Drive ${Drive}: is already occupied." }
    & subst.exe "${Drive}:" $Target
    if ($LASTEXITCODE -ne 0) { throw "Cannot create drive alias ${Drive}:" }
}

Ensure-DriveAlias 'P' $ProjectRoot
Ensure-DriveAlias 'U' $EngineRoot

if ($GenerateProjectFiles) {
    & 'U:\Engine\Build\BatchFiles\Build.bat' -ProjectFiles "-Project=$ProjectRoot\ProjectMegaSample.uproject" -Game -Engine -2022
} else {
    & 'U:\Engine\Build\BatchFiles\Build.bat' ProjectMegaSampleEditor Win64 Development '-Project=P:\ProjectMegaSample.uproject' -WaitMutex -NoHotReloadFromIDE -NoUBA
}
if ($LASTEXITCODE -ne 0) { throw "UnrealBuildTool failed with exit code $LASTEXITCODE." }
