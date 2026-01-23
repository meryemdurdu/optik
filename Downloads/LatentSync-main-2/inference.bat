@echo off
REM LatentSync 1.5 - Windows Inference Script
REM Usage: inference.bat [video_path] [audio_path] [output_path]
REM Example: inference.bat input.mp4 audio.wav output.mp4

setlocal enabledelayedexpansion

REM Default values
set VIDEO_PATH=assets\demo1_video.mp4
set AUDIO_PATH=assets\demo1_audio.wav
set OUTPUT_PATH=video_out.mp4

REM Override with command line arguments if provided
if not "%~1"=="" set VIDEO_PATH=%~1
if not "%~2"=="" set AUDIO_PATH=%~2
if not "%~3"=="" set OUTPUT_PATH=%~3

echo ========================================
echo LatentSync 1.5 - Inference
echo ========================================
echo Video:  %VIDEO_PATH%
echo Audio:  %AUDIO_PATH%
echo Output: %OUTPUT_PATH%
echo ========================================

python -m scripts.inference ^
    --unet_config_path "configs/unet/stage2.yaml" ^
    --inference_ckpt_path "checkpoints/latentsync_unet.pt" ^
    --inference_steps 20 ^
    --guidance_scale 1.5 ^
    --enable_deepcache ^
    --video_path "%VIDEO_PATH%" ^
    --audio_path "%AUDIO_PATH%" ^
    --video_out_path "%OUTPUT_PATH%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Inference completed successfully!
    echo Output saved to: %OUTPUT_PATH%
    echo ========================================
) else (
    echo.
    echo ========================================
    echo ERROR: Inference failed with code %ERRORLEVEL%
    echo ========================================
)

endlocal
