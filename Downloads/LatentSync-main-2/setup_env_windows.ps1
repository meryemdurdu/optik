# LatentSync 1.5 Windows Setup Script
# Run this script in PowerShell with Administrator privileges if needed

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "LatentSync 1.5 - Windows Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Check if conda is available
$condaPath = Get-Command conda -ErrorAction SilentlyContinue
if (-not $condaPath) {
    Write-Host "ERROR: Conda is not installed or not in PATH" -ForegroundColor Red
    Write-Host "Please install Miniconda or Anaconda first: https://docs.conda.io/en/latest/miniconda.html" -ForegroundColor Yellow
    exit 1
}

Write-Host "`n[1/5] Creating conda environment..." -ForegroundColor Green
conda create -y -n latentsync python=3.10.13

Write-Host "`n[2/5] Activating environment..." -ForegroundColor Green
# Initialize conda for PowerShell if not already done
conda init powershell 2>$null
# Activate the environment
conda activate latentsync

Write-Host "`n[3/5] Installing ffmpeg via conda..." -ForegroundColor Green
conda install -y -c conda-forge ffmpeg

Write-Host "`n[4/5] Installing Python dependencies..." -ForegroundColor Green
pip install -r requirements_windows.txt

Write-Host "`n[5/5] Downloading LatentSync 1.5 checkpoints from HuggingFace..." -ForegroundColor Green
# Create checkpoints directory if not exists
if (-not (Test-Path "checkpoints")) {
    New-Item -ItemType Directory -Path "checkpoints" | Out-Null
}
if (-not (Test-Path "checkpoints/whisper")) {
    New-Item -ItemType Directory -Path "checkpoints/whisper" | Out-Null
}

huggingface-cli download ByteDance/LatentSync-1.5 whisper/tiny.pt --local-dir checkpoints
huggingface-cli download ByteDance/LatentSync-1.5 latentsync_unet.pt --local-dir checkpoints

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Setup completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "`nTo activate the environment, run:" -ForegroundColor Yellow
Write-Host "  conda activate latentsync" -ForegroundColor White
Write-Host "`nTo run inference:" -ForegroundColor Yellow
Write-Host "  .\inference.bat" -ForegroundColor White
Write-Host "  OR" -ForegroundColor Yellow
Write-Host "  python -m scripts.inference --help" -ForegroundColor White
