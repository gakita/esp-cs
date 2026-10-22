# ==============================================================
# Vanta — rebuild script
# Rode da raiz do projeto (onde está o CMakeLists.txt).
#   Uso:  .\rebuild.ps1
#         .\rebuild.ps1 -Debug      # build Debug ao inves de Release
#         .\rebuild.ps1 -NoClean    # incremental, sem apagar build/
#         .\rebuild.ps1 -Run        # inicia o .exe depois do build
# ==============================================================

param(
    [switch]$Debug,
    [switch]$NoClean,
    [switch]$Run
)

$ErrorActionPreference = 'Stop'
$proj = (Get-Location).Path
$config = if ($Debug) { 'Debug' } else { 'Release' }

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Vanta rebuild" -ForegroundColor Cyan
Write-Host "   dir    : $proj"
Write-Host "   config : $config"
Write-Host "   clean  : $(-not $NoClean)"
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# ---- sanity ----
if (-not (Test-Path (Join-Path $proj 'CMakeLists.txt'))) {
    Write-Host "ERRO: nao achei CMakeLists.txt. Rode isso da RAIZ do projeto." -ForegroundColor Red
    exit 1
}

# ---- 1) mata instancias rodando ----
$targets = @('vanta', 'Vanta', 'cs_cheat', 'VantaClient')
foreach ($name in $targets) {
    $running = Get-Process -Name $name -ErrorAction SilentlyContinue
    if ($running) {
        Write-Host "Matando processo em execucao: $name (pid $($running.Id))" -ForegroundColor Yellow
        $running | Stop-Process -Force
        Start-Sleep -Milliseconds 300
    }
}

# ---- 2) clean ----
$buildDir = Join-Path $proj 'build'
if (-not $NoClean -and (Test-Path $buildDir)) {
    Write-Host "Limpando build/..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

# ---- 3) configure ----
Write-Host ""
Write-Host "--- cmake configure ---" -ForegroundColor Cyan
cmake -B build -A x64
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERRO: cmake configure falhou (exit $LASTEXITCODE)" -ForegroundColor Red
    exit $LASTEXITCODE
}

# ---- 4) build ----
Write-Host ""
Write-Host "--- cmake build ($config) ---" -ForegroundColor Cyan
$cores = [Environment]::ProcessorCount
cmake --build build --config $config --parallel $cores
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERRO: build falhou (exit $LASTEXITCODE)" -ForegroundColor Red
    exit $LASTEXITCODE
}

# ---- 5) locate output ----
$candidates = @(
    (Join-Path $buildDir "$config\vanta.exe"),
    (Join-Path $buildDir "$config\Vanta.exe"),
    (Join-Path $buildDir "$config\cs_cheat.exe"),
    (Join-Path $buildDir "$config\VantaClient.exe")
)
$exe = $null
foreach ($c in $candidates) { if (Test-Path $c) { $exe = $c; break } }

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host " BUILD OK" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
if ($exe) {
    $info = Get-Item $exe
    Write-Host "   exe   : $exe"
    Write-Host "   size  : $([math]::Round($info.Length / 1KB, 1)) KB"
    Write-Host "   built : $($info.LastWriteTime)"
} else {
    Write-Host "   (nao localizei o .exe automaticamente — procure em $buildDir\$config\)"
}
Write-Host ""

# ---- 6) run? ----
if ($Run -and $exe) {
    Write-Host "Iniciando $exe ..." -ForegroundColor Yellow
    Start-Process -FilePath $exe -Verb RunAs
}