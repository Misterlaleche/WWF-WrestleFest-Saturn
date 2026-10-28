param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $repoRoot '..\toolchains\host-env.ps1')
$env:WF_ZIG_EXE = (Resolve-Path (Join-Path $repoRoot '..\.zig13\ziglang\zig.exe')).Path
$reportRoot = Join-Path $PSScriptRoot 'local-20261026\action-interleaved'

function RunCase([string]$name, [string]$selector, [int]$steps, [string[]]$extra) {
    $arguments = @('--selector', $selector, '--all-characters', '--sequence-only',
                   '--steps', [string]$steps) + $extra + @('--report', (Join-Path $reportRoot $name))
    & $taskPython tests/r18/action_reversal_family_verify_rom.py @arguments | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "$name failed: $LASTEXITCODE" }
    Write-Output "PASS $name"
}

Push-Location (Join-Path $repoRoot 'src\runtime')
try {
    $air = @('--interleave-physics', '--interleave-resolver')
    RunCase 'action00-base.json' '0x00' 80 @('--idle-descriptor', 'base')
    RunCase 'action00-alternate.json' '0x00' 80 @('--idle-descriptor', 'alternate')
    foreach ($edge in @('left','right','left-high','right-high','apron-left','apron-right','outer-left','outer-right')) {
        RunCase "action00-$edge.json" '0x00' 80 @('--idle-descriptor', 'base', '--idle-edge', $edge)
    }
    foreach ($kind in @('ground8', 'ground9', 'counter61', 'miss')) {
        RunCase "action0c-$kind.json" '0x0c' 512 ($air + @('--jump-contact', $kind))
        RunCase "action10-$kind.json" '0x10' 512 ($air + @('--jump10-contact', $kind))
    }
    foreach ($kind in @('ground8', 'ground9')) {
        RunCase "action0c-victim-$kind.json" '0x0c' 512 ($air + @('--jump-contact', $kind, '--victim-follow', '256'))
        RunCase "action10-victim-$kind.json" '0x10' 512 ($air + @('--jump10-contact', $kind, '--victim-follow', '256'))
    }
    RunCase 'action0c-counter-return.json' '0x0c' 512 ($air + @('--jump-contact', 'counter61', '--victim-follow', '256'))
    RunCase 'action10-counter-return.json' '0x10' 512 ($air + @('--jump10-contact', 'counter61', '--victim-follow', '256'))
    foreach ($kind in @('route0', 'route1', 'route2', 'miss')) {
        RunCase "action0d-$kind.json" '0x0d' 512 ($air + @('--jump-attack', $kind))
    }
    foreach ($kind in @('route0', 'route1', 'route2')) {
        RunCase "action0d-victim-$kind.json" '0x0d' 512 ($air + @('--jump-attack', $kind, '--victim-follow', '512'))
    }
    foreach ($kind in @('hit', 'miss', 'special54')) {
        RunCase "action0e-$kind.json" '0x0e' 512 ($air + @('--running-contact', $kind))
    }
    RunCase 'action0e-victim-hit.json' '0x0e' 512 ($air + @('--running-contact', 'hit', '--victim-follow', '512'))
    RunCase 'action11.json' '0x11' 256 $air
    RunCase 'action11-downed-recovery.json' '0x11' 256 ($air + @('--linked-downed'))
    RunCase 'action11-victim-recovery.json' '0x11' 256 ($air + @('--linked-downed', '--victim-follow', '256'))
    RunCase 'action11-active-contact.json' '0x11' 256 ($air + @('--linked-active'))
    RunCase 'action12-fall.json' '0x12' 256 $air
    RunCase 'action12-impact.json' '0x12' 256 ($air + @('--raw12-downed'))
    RunCase 'action12-active.json' '0x12' 256 ($air + @('--raw12-active'))
    RunCase 'action12-downed-recovery.json' '0x12' 512 ($air + @('--raw12-downed', '--follow-impact'))
    RunCase 'action12-victim-recovery.json' '0x12' 512 ($air + @('--raw12-downed', '--follow-impact', '--victim-follow', '256'))
    RunCase 'action12-active-recovery.json' '0x12' 512 ($air + @('--raw12-active', '--follow-impact'))
    RunCase 'action13-fall.json' '0x13' 256 $air
    RunCase 'action13-impact.json' '0x13' 256 ($air + @('--raw13-downed'))
    RunCase 'action13-recovery.json' '0x13' 512 ($air + @('--raw13-downed', '--follow-impact'))
    RunCase 'action13-victim-recovery.json' '0x13' 512 ($air + @('--raw13-downed', '--follow-impact', '--victim-follow', '256'))
    RunCase 'action14.json' '0x14' 256 $air
    RunCase 'action14-downed-recovery.json' '0x14' 256 ($air + @('--linked-downed'))
    RunCase 'action14-victim-recovery.json' '0x14' 256 ($air + @('--linked-downed', '--victim-follow', '256'))
    RunCase 'action14-active-miss.json' '0x14' 256 ($air + @('--linked-active'))
    foreach ($mode in 0..3) {
        $modeArgs = $air + @('--raw35-mode', [string]$mode, '--raw35-target-x', '0x2c0')
        RunCase "action35-mode$mode.json" '0x35' 256 $modeArgs
        RunCase "action35-victim-mode$mode.json" '0x35' 256 ($modeArgs + @('--victim-follow', '256'))
        RunCase "action35-active-mode$mode.json" '0x35' 300 ($modeArgs + @('--raw35-active'))
    }
    foreach ($mode in 2..3) {
        RunCase "action35-rope-mode$mode.json" '0x35' 300 (
            $air + @('--raw35-mode', [string]$mode, '--raw35-target-x', '0x310'))
    }
    RunCase 'action44-success.json' '0x44' 200 @('--interleave-resolver', '--raw44-index', '0')
    foreach ($index in 1..4) {
        RunCase "action44-success-index$index.json" '0x44' 200 @(
            '--interleave-resolver', '--raw44-index', [string]$index, '--raw44-force-success')
    }
    foreach ($index in 1..3) {
        RunCase "action44-miss-index$index.json" '0x44' 200 @(
            '--interleave-resolver', '--raw44-index', [string]$index, '--raw44-force-fail')
    }
    RunCase 'action44-miss.json' '0x44' 200 @('--interleave-resolver', '--raw44-index', '4', '--raw44-force-fail')
    RunCase 'action8e.json' '0x8e' 653 @('--interleave-resolver')
} finally {
    Pop-Location
}

& $taskPython (Join-Path $PSScriptRoot 'action-selector-inventory.py') | Out-Null
if ($LASTEXITCODE -ne 0) { throw "action inventory failed: $LASTEXITCODE" }
Write-Output 'PASS action-selector-inventory.json'
