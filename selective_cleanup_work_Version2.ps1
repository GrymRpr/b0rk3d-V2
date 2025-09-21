# SelectiveCleanupWork.ps1
# Scans "src" folder and asks individually about each non-cpp/hpp file

$workFolder = ".\src"

# Ensure src folder exists
if (-not (Test-Path $workFolder)) {
    Write-Error "src folder not found at $workFolder"
    exit
}

Write-Host "--- Scanning src folder for non-cpp/hpp files ---" -ForegroundColor Cyan

# Get all files in src folder (recursively)
$allFiles = Get-ChildItem -Path $workFolder -File -Recurse

# Filter files that are NOT .cpp or .hpp
$nonCppHppFiles = $allFiles | Where-Object { $_.Extension -ne '.cpp' -and $_.Extension -ne '.hpp' }

if ($nonCppHppFiles.Count -eq 0) {
    Write-Host "No non-cpp/hpp files found. All files are .cpp or .hpp files." -ForegroundColor Green
} else {
    Write-Host "Found $($nonCppHppFiles.Count) non-cpp/hpp files." -ForegroundColor Yellow
    
    $deletedCount = 0
    $skippedCount = 0
    
    foreach ($file in $nonCppHppFiles) {
        Write-Host "`nFile: $($file.FullName)" -ForegroundColor Yellow
        Write-Host "Extension: $($file.Extension)" -ForegroundColor Gray
        
        $response = Read-Host "Delete this file? (y/n)"
        
        if ($response -eq 'y') {
            try {
                Remove-Item -Path $file.FullName -Force -ErrorAction Stop
                Write-Host "  DELETED: $($file.Name)" -ForegroundColor Green
                $deletedCount++
            } catch {
                Write-Error "  DELETE FAILED: $($file.Name). Error: $_"
            }
        } else {
            Write-Host "  SKIPPED: $($file.Name)" -ForegroundColor Gray
            $skippedCount++
        }
    }
    
    Write-Host "`nSummary: $deletedCount files deleted, $skippedCount files skipped." -ForegroundColor Cyan
}