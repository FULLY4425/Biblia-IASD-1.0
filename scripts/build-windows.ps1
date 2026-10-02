$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$obsSource = Join-Path $taskRoot 'work-ci/obs-studio'
$sdkRoot = Join-Path $taskRoot 'work-ci/sdk'
$obsBuild = Join-Path $taskRoot 'work-ci/obs-build'
$pluginBuild = Join-Path $taskRoot 'build'
$packageRoot = Join-Path $taskRoot 'dist'
$vsLocator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsVersion = & $vsLocator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
if (-not $vsVersion) { throw 'No se encontró Visual Studio con herramientas C++.' }
$taskGenerator = if (([version]$vsVersion).Major -ge 18) { 'Visual Studio 18 2026' } else { 'Visual Studio 17 2022' }

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program terminó con código $LASTEXITCODE" }
}

if (-not (Test-Path -LiteralPath "$obsSource/CMakeLists.txt")) {
    throw 'Falta work-ci/obs-studio. Clona obsproject/obs-studio, tag 32.2.2, incluyendo submódulos.'
}

Invoke-Checked cmake @('-S', $obsSource, '-B', $obsBuild, '-G', $taskGenerator, '-A', 'x64',
    '-DENABLE_PLUGINS=OFF', '-DENABLE_FRONTEND=OFF', '-DENABLE_BROWSER=OFF', '-DENABLE_SCRIPTING=OFF',
    '-DOBS_VERSION_OVERRIDE=32.2.2')
Invoke-Checked cmake @('--build', $obsBuild, '--config', 'Release', '--target', 'obs-frontend-api', '--parallel', '4')
Invoke-Checked cmake @('--install', $obsBuild, '--config', 'Release', '--component', 'Development', '--prefix', $sdkRoot)

$qtPrefix = Join-Path $obsSource '.deps/obs-deps-qt6-2026-07-15-x64'
$depsPrefix = Join-Path $obsSource '.deps/obs-deps-2026-07-15-x64'
$prefix = "$sdkRoot;$qtPrefix;$depsPrefix"
Invoke-Checked cmake @('-S', $taskRoot, '-B', $pluginBuild, '-G', $taskGenerator, '-A', 'x64', "-DCMAKE_PREFIX_PATH=$prefix", '-DBUILD_TESTING=ON')
Invoke-Checked cmake @('--build', $pluginBuild, '--config', 'Release', '--parallel', '4')

$env:PATH = "$qtPrefix/bin;$depsPrefix/bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$qtPrefix/plugins"
Invoke-Checked ctest @('--test-dir', $pluginBuild, '-C', 'Release', '--output-on-failure')
Invoke-Checked python @("$taskRoot/tests/verify_data.py")
Invoke-Checked cmake @('--install', $pluginBuild, '--config', 'Release', '--prefix', $packageRoot)
Copy-Item -LiteralPath "$taskRoot/LICENSE", "$taskRoot/README.md" -Destination $packageRoot
Get-FileHash -LiteralPath "$packageRoot/obs-plugins/64bit/obs-biblia.dll" -Algorithm SHA256 | Format-List
