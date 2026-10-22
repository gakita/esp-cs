# Fetches ImGui 1.90.9 release and extracts the files CMakeLists needs
# into src/imgui/ and src/imgui/backends/.
$ErrorActionPreference = "Stop"
$version = "1.90.9"
$url  = "https://github.com/ocornut/imgui/archive/refs/tags/v$version.zip"
$tmp  = Join-Path $env:TEMP "imgui-$version.zip"
$dest = Join-Path (Get-Location) "src\imgui"

Write-Host "Downloading ImGui $version..."
Invoke-WebRequest -Uri $url -OutFile $tmp
$extract = Join-Path $env:TEMP "imgui-extract-$version"
if (Test-Path $extract) { Remove-Item -Recurse -Force $extract }
Expand-Archive -Path $tmp -DestinationPath $extract -Force

$src = Join-Path $extract "imgui-$version"
$files = @(
    "imgui.h","imgui.cpp","imgui_draw.cpp","imgui_tables.cpp",
    "imgui_widgets.cpp","imgui_internal.h","imstb_rectpack.h",
    "imstb_textedit.h","imstb_truetype.h","imconfig.h"
)
foreach ($f in $files) { Copy-Item (Join-Path $src $f) (Join-Path $dest $f) -Force }
$backends = @("imgui_impl_dx11.h","imgui_impl_dx11.cpp","imgui_impl_win32.h","imgui_impl_win32.cpp")
foreach ($f in $backends) { Copy-Item (Join-Path $src "backends\$f") (Join-Path $dest "backends\$f") -Force }

Remove-Item $tmp
Remove-Item -Recurse -Force $extract
Write-Host "ImGui ready in src/imgui/"