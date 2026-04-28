# Wallpapi Wallpaper Selector
Add-Type -AssemblyName System.Windows.Forms
$FileBrowser = New-Object System.Windows.Forms.OpenFileDialog -Property @{
    InitialDirectory = [System.IO.Path]::GetFullPath(".\wallpapers")
    Filter = "Video Files (*.mp4;*.mkv;*.avi)|*.mp4;*.mkv;*.avi|All files (*.*)|*.*"
    Title = "Select Wallpaper Video"
}

$DialogResult = $FileBrowser.ShowDialog()

if ($DialogResult -eq [System.Windows.Forms.DialogResult]::OK) {
    $SelectedFile = $FileBrowser.FileName
    Write-Host "Selected: $SelectedFile"
    
    # Try to find wp-cli.exe in common locations
    $CliPath = ".\build\Release\wp-cli.exe"
    if (-not (Test-Path $CliPath)) {
        $CliPath = ".\wp-cli.exe"
    }

    if (Test-Path $CliPath) {
        & $CliPath set-video $SelectedFile
    } else {
        Write-Error "Could not find wp-cli.exe. Please build the project first."
        pause
    }
}
