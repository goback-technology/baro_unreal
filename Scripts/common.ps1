$BaroScriptDir = $PSScriptRoot
$BaroRoot      = Split-Path -Parent $BaroScriptDir
$BaroEnvPath   = Join-Path $BaroRoot ".env"
$BaroEnv       = @{}

if (Test-Path -LiteralPath $BaroEnvPath) {
    foreach ($rawLine in Get-Content -LiteralPath $BaroEnvPath) {
        $line = $rawLine.Trim()
        if ([string]::IsNullOrWhiteSpace($line) -or $line.StartsWith("#")) {
            continue
        }

        $match = [regex]::Match($line, '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)\s*$')
        if (-not $match.Success) {
            continue
        }

        $key = $match.Groups[1].Value
        $value = $match.Groups[2].Value.Trim()

        if (($value.StartsWith('"') -and $value.EndsWith('"')) -or
            ($value.StartsWith("'") -and $value.EndsWith("'"))) {
            $value = $value.Substring(1, $value.Length - 2)
        }

        $BaroEnv[$key] = $value
    }
}

function Get-BaroSetting {
    param(
        [Parameter(Mandatory = $true)] [string]$Name,
        [string]$Default = ""
    )

    if ($BaroEnv.ContainsKey($Name) -and -not [string]::IsNullOrWhiteSpace($BaroEnv[$Name])) {
        return $BaroEnv[$Name]
    }

    $processValue = [Environment]::GetEnvironmentVariable($Name, "Process")
    if (-not [string]::IsNullOrWhiteSpace($processValue)) {
        return $processValue
    }

    return $Default
}

function Get-BaroBoolSetting {
    param(
        [Parameter(Mandatory = $true)] [string]$Name,
        [bool]$Default = $false
    )

    $value = (Get-BaroSetting -Name $Name -Default "").Trim().ToLowerInvariant()
    switch -Regex ($value) {
        '^(1|true|yes|y|on)$'  { return $true }
        '^(0|false|no|n|off)$' { return $false }
        default                { return $Default }
    }
}

function Assert-BaroFile {
    param(
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string]$Message
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Message`: $Path"
    }
}

$BaroEngine     = Get-BaroSetting -Name "UE_PATH" -Default "C:\Program Files\Epic Games\UE_5.8"
$BaroProject    = Get-BaroSetting -Name "PROJECT_FILE" -Default (Join-Path $BaroRoot "baro_unreal.uproject")
$BaroDefaultMap = Get-BaroSetting -Name "DEFAULT_MAP" -Default "/Game/simulator/LV_Park_sim_01"
