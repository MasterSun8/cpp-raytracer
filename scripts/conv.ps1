cd ..
Get-ChildItem -Filter *.ppm | ForEach-Object {
    echo "Converting $_ to PNG format..."
    $pngName = $_.BaseName + ".png"
    magick $_.FullName $pngName
}
Write-Host "Conversion complete: All .ppm files have been converted to .png."
