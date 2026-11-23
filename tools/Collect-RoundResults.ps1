[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string[]]$RoundFolders,

    [Parameter(Mandatory)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
$seen = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$rounds = foreach ($folder in $RoundFolders) {
    $dir = Get-Item -LiteralPath $folder
    if (-not $dir.PSIsContainer) {
        throw "Expected a round folder: $folder"
    }
    $summaries = @(Get-ChildItem -LiteralPath $dir.FullName -File -Recurse -Filter 'summary.csv' | Sort-Object FullName)
    if ($summaries.Count -eq 0) {
        throw "No evaluation summary found: $folder"
    }
    foreach ($summary in $summaries) {
        if (-not $seen.Add($summary.FullName)) {
            throw "Evaluation supplied more than once: $($summary.FullName)"
        }
        $rows = @(Import-Csv -LiteralPath $summary.FullName -Encoding UTF8)
        if ($rows.Count -ne 1 -or $null -eq $rows[0].incomplete_jobs) {
            throw "Invalid evaluation summary: $($summary.FullName)"
        }
        $rows[0].run_dir = $summary.DirectoryName
        $rows[0] | Select-Object @{Name = 'round'; Expression = { $dir.Name }}, *
    }
}

$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
if ($seen.Contains($output) -or [IO.Path]::GetFileName($output) -ieq 'summary.csv') {
    throw 'Choose a different name for the combined CSV.'
}
$rounds | Export-Csv -LiteralPath $output -Encoding UTF8 -NoTypeInformation
