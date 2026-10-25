# setup_gcc.ps1 - gcc(WinLibs) 를 C:\Users\Public\vsm-gcc 에 받는다 (setup_guide.md 1단계)
#
# 하는 일: 내려받기 -> SHA256 대조 -> 압축 풀기 -> gcc.exe · mingw32-make.exe 확인 -> zip 삭제
# 이미 받아 두었으면 아무것도 하지 않는다. 어느 단계든 실패하면 exit 1.
#
# 위치를 프로젝트 밖 영문 경로로 고정하는 이유: gcc 를 한글이 든 경로에 풀면 링크가 깨진다
# (gcc.exe · cc1.exe 는 UTF-8, collect2 · ld 는 시스템 코드 페이지라 ld 가 crt2.o · 라이브러리 경로를 못 연다).
# 프로젝트 경로의 한글은 괜찮다 - 빌드가 상대 경로(src/*.c)만 넘기기 때문이다.
#
# 실행: powershell -NoProfile -ExecutionPolicy Bypass -File setup\setup_gcc.ps1
#       뒤에 -Offline 을 붙이면 내려받지 않고 C:\Users\Public\vsm-gcc 에 둔 zip 만 쓴다 (브라우저로 따로 받은 경우)

param([switch]$Offline)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 받을 파일 - 바꿀 때는 이름 · 주소 · SHA256 셋을 같이 바꾼다
$ZipName = 'winlibs-i686-posix-dwarf-gcc-15.2.0-mingw-w64msvcrt-14.0.0-r7.zip'
$Url     = "https://github.com/brechtsanders/winlibs_mingw/releases/download/15.2.0posix-14.0.0-msvcrt-r7/$ZipName"
$Sha256  = 'ED7FB2DB387834849AF00216A609D36B3D65730BA28CA73C7D759360EDAEB913'

# 받을 곳 - config.bat · verify_build.ps1 과 같은 곳이어야 한다 (바꿀 때는 셋을 같이)
$GccHome = Join-Path $env:PUBLIC 'vsm-gcc'
$Gcc     = Join-Path $GccHome 'mingw32'
$Bin     = Join-Path $Gcc 'bin'
$Zip     = Join-Path $GccHome $ZipName
$Marker  = Join-Path $GccHome '.setup-ok'    # 압축을 끝까지 푼 뒤에만 쓴다 - 풀다 만 폴더를 완료로 읽지 않기 위해
$Sys32   = Join-Path $env:SystemRoot 'System32'

function Fail([string]$msg)
{
    Write-Output "[setup] 실패: $msg"
    exit 1
}

function Test-Installed
{
    (Test-Path -LiteralPath $Marker) -and
    ((Get-Content -LiteralPath $Marker -Raw).Trim() -eq $ZipName) -and
    (Test-Path -LiteralPath (Join-Path $Bin 'gcc.exe')) -and
    (Test-Path -LiteralPath (Join-Path $Bin 'mingw32-make.exe'))
}

function Show-Version
{
    Write-Output ("[setup] " + (& (Join-Path $Bin 'gcc.exe') --version)[0])
    Write-Output ("[setup] " + (& (Join-Path $Bin 'mingw32-make.exe') --version)[0])
}

# 0. 받을 곳이 영문 경로인지 - 아니면 링크가 깨지므로 받기 전에 멈춘다
if ([string]::IsNullOrEmpty($env:PUBLIC)) { Fail '환경 변수 PUBLIC 이 없습니다 (보통 C:\Users\Public)' }
if ($GccHome -match '[^\x20-\x7E]')
{
    Fail "받을 곳 $GccHome 에 영문이 아닌 문자가 있습니다 - gcc 를 한글 경로에 풀면 링크가 깨집니다. 보고하십시오."
}

if (Test-Installed)
{
    Write-Output "[setup] 이미 받아 두었습니다: $Bin"
    Show-Version
    exit 0
}

New-Item -ItemType Directory -Force -Path $GccHome | Out-Null

# 1. 내려받기 - 같은 파일(SHA256 일치)이 이미 있으면 건너뛴다
if ((Test-Path -LiteralPath $Zip) -and ((Get-FileHash -Algorithm SHA256 -LiteralPath $Zip).Hash -ne $Sha256))
{
    Write-Output "[setup] $Zip 의 SHA256 이 다릅니다 - 지우고 다시 받습니다"
    Remove-Item -LiteralPath $Zip -Force
}
if (-not (Test-Path -LiteralPath $Zip))
{
    if ($Offline)
    {
        Fail "-Offline 인데 $Zip 이 없습니다. 아래 주소에서 받아 $GccHome 에 두고 다시 실행하십시오.`n  $Url"
    }
    Write-Output "[setup] 내려받는 중 (약 268MB · 몇 분 걸림): $Url"
    $curl = Join-Path $Sys32 'curl.exe'
    if (Test-Path -LiteralPath $curl)
    {
        & $curl -sS -L --fail --retry 3 -o $Zip $Url
        $code = $LASTEXITCODE
        if ($code -ne 0)
        {
            if (Test-Path -LiteralPath $Zip) { Remove-Item -LiteralPath $Zip -Force }
            Fail "curl exit $code - 네트워크 · 프록시를 확인하십시오 (setup_guide.md 「문제 해결」)"
        }
    }
    else
    {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        $ProgressPreference = 'SilentlyContinue'
        try
        {
            Invoke-WebRequest -Uri $Url -OutFile $Zip -UseBasicParsing
        }
        catch
        {
            if (Test-Path -LiteralPath $Zip) { Remove-Item -LiteralPath $Zip -Force }
            Fail "내려받기 실패 - $($_.Exception.Message)"
        }
    }
}

# 2. SHA256 대조 - 다르면 지우고 멈춘다 (다른 파일로 바꿔 넣지 않는다)
$actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $Zip).Hash
if ($actual -ne $Sha256)
{
    Remove-Item -LiteralPath $Zip -Force
    Fail "SHA256 불일치 - 기대 $Sha256 / 실제 $actual (받은 파일은 지웠습니다)"
}
Write-Output '[setup] SHA256 일치'

# 3. 압축 풀기 - 지난번에 풀다 만 폴더가 있으면 지우고 처음부터
if (Test-Path -LiteralPath $Gcc) { Remove-Item -LiteralPath $Gcc -Recurse -Force }
if (Test-Path -LiteralPath $Marker) { Remove-Item -LiteralPath $Marker -Force }
Write-Output '[setup] 압축 푸는 중 (1~3분)'
$tar = Join-Path $Sys32 'tar.exe'
Push-Location $GccHome
try
{
    if (Test-Path -LiteralPath $tar)
    {
        & $tar -xf $ZipName
        if ($LASTEXITCODE -ne 0) { Fail "tar exit $LASTEXITCODE" }
    }
    else
    {
        Expand-Archive -LiteralPath $Zip -DestinationPath $GccHome -Force
    }
}
finally
{
    Pop-Location
}

# 4. 확인 - 실행 파일 둘이 있어야 완료
if (-not ((Test-Path -LiteralPath (Join-Path $Bin 'gcc.exe')) -and (Test-Path -LiteralPath (Join-Path $Bin 'mingw32-make.exe'))))
{
    Fail "압축은 풀었지만 $Bin 에 gcc.exe · mingw32-make.exe 가 없습니다"
}
Set-Content -LiteralPath $Marker -Value $ZipName -Encoding ASCII
Remove-Item -LiteralPath $Zip -Force

Write-Output "[setup] 완료: $Bin"
Show-Version
exit 0
