param([string]$RomPath = '', [switch]$CompareOnly)
$ErrorActionPreference = 'Stop'
$packageRoot = Split-Path -Parent $PSScriptRoot
$gameExe = Join-Path $packageRoot 'DynaBrothers1AOT.exe'
if (-not $RomPath) {
    Add-Type -AssemblyName System.Windows.Forms
    $picker = New-Object System.Windows.Forms.OpenFileDialog
    $picker.Title = 'Select your Dyna Brothers cartridge dump'
    $picker.Filter = 'Mega Drive ROM (*.md;*.bin)|*.md;*.bin|All files|*.*'
    if ($picker.ShowDialog() -ne 'OK') { return }
    $RomPath = $picker.FileName
}
$RomPath = (Resolve-Path -LiteralPath $RomPath).Path
if (-not (Test-Path -LiteralPath $gameExe)) { throw 'Extract the entire portable package first.' }
$runRoot = Join-Path $PSScriptRoot ('results-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $runRoot | Out-Null
$oldProfile = $env:GENESIS_USER_DIR
try {
    foreach ($test in @('DynaDb1VideoTest.exe', 'DynaDb1visibleTest.exe')) {
        & (Join-Path $PSScriptRoot $test)
        if ($LASTEXITCODE) { throw "Renderer contract failed: $test" }
    }
    $states = @()
    foreach ($mode in @('off', '16:9')) {
        $name = if ($mode -eq 'off') { 'original' } else { 'enhanced' }
        $folder = Join-Path $runRoot $name
        New-Item -ItemType Directory -Path (Join-Path $folder 'profile') -Force | Out-Null
        $env:GENESIS_USER_DIR = Join-Path $folder 'profile'
        Push-Location -LiteralPath $folder
        try {
            $gameArguments = @($RomPath, '--no-launcher', '--benchmark', '6100', '--widescreen', $mode,
                '--input-script', (Join-Path $PSScriptRoot 'db1-16x9.input'))
            $ErrorActionPreference = 'Continue'
            & $gameExe @gameArguments *> run.log
            $result = $LASTEXITCODE
            $ErrorActionPreference = 'Stop'
            if ($result) { throw "Native test failed ($result); see $folder/run.log" }
            $log = Get-Content -LiteralPath 'run.log' -Raw
            $state = [regex]::Match($log, '"state_fnv1a64"\s*:\s*"([A-Fa-f0-9]+)"').Groups[1].Value
            $audio = [regex]::Match($log, '"audio_state_fnv1a64"\s*:\s*"([A-Fa-f0-9]+)"').Groups[1].Value
            if (-not $state -or -not $audio) { throw 'Missing native state evidence.' }
            $states += "$state/$audio"
            # The runner uses its isolated profile as the working directory.
            if (-not (Test-Path -LiteralPath (Join-Path $env:GENESIS_USER_DIR 'expanded-world.png'))) {
                throw 'Missing scene capture.'
            }
        } finally { Pop-Location }
    }
    if ($states[0] -ne $states[1]) { throw 'Original/Enhanced guest state mismatch.' }
    $identity = (Get-FileHash -LiteralPath $gameExe -Algorithm SHA256).Hash
    @("PASS: identical CPU/RAM/VDP and audio state after 6100 frames.",
      "State: $($states[0])", "Executable SHA256: $identity",
      'Synthetic contracts also cover all 253 valid actor IDs, UI priority and host clipping.') |
        Set-Content -LiteralPath (Join-Path $runRoot 'RESULT.txt') -Encoding UTF8
    Write-Host "PASS. Captures and logs: $runRoot"
    if (-not $CompareOnly) {
        $env:GENESIS_USER_DIR = Join-Path $runRoot 'interactive-profile'
        New-Item -ItemType Directory -Path $env:GENESIS_USER_DIR | Out-Null
        Write-Host 'Starting the interactive 16:9 test. Esc opens settings; Back/Mode opens them on a mapped controller.'
        & $gameExe $RomPath --no-launcher --widescreen '16:9'
    }
} finally { $env:GENESIS_USER_DIR = $oldProfile }
