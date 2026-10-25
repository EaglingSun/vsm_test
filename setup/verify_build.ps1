# verify_build.ps1 - 환경 세팅 · 빌드 성공을 확인한다 (setup_guide.md 2단계)
#
# 네 항목이 모두 [PASS] 면 exit 0, 하나라도 [FAIL] 이면 exit 1.
#   1. config.bat 이 성공한다
#   2. gcc 가 setup_gcc.ps1 이 받은 C:\Users\Public\vsm-gcc\mingw32\bin 의 것으로 잡힌다
#   3. config.bat 을 먼저 부르지 않아도 make clean · make 가 성공하고 (make.cmd 가 config.bat 을 부른다),
#      빌드 로그에 옛 실습 도구(STATIC · CT) 경로가 없다
#   4. vsm_demo.exe 가 만들어진다
#
# 실행: powershell -NoProfile -ExecutionPolicy Bypass -File setup\verify_build.ps1

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$Root     = Split-Path -Parent $PSScriptRoot
# gcc 위치 - setup_gcc.ps1 · config.bat 과 같은 곳이어야 한다 (바꿀 때는 셋을 같이)
$LocalGcc = Join-Path $env:PUBLIC 'vsm-gcc\mingw32\bin\gcc.exe'
$Exe      = Join-Path $Root 'vsm_demo.exe'
$script:failed = 0

function Report([string]$name, [bool]$ok, [string[]]$detail)
{
    if ($ok) { Write-Output "[PASS] $name" } else { Write-Output "[FAIL] $name"; $script:failed++ }
    foreach ($line in $detail) { if ($line) { Write-Output "       $line" } }
}

Push-Location $Root
try
{
    # 1. config.bat
    #    .\ 를 붙인다 - Claude Code 는 NoDefaultCurrentDirectoryInExePath=1 을 걸어 cmd 가 현재 폴더를 찾지 않는다
    $cfg = @(& cmd.exe /d /c "call .\config.bat 2>&1")
    Report '1. config.bat 실행' ($LASTEXITCODE -eq 0) $cfg

    # 2. 어떤 gcc 가 잡히나 - where 의 첫 줄이 실제로 쓰이는 gcc 다
    $where = @(& cmd.exe /d /c "call .\config.bat >nul 2>&1 && where gcc 2>&1")
    $first = if ($where.Count -gt 0) { "$($where[0])".Trim() } else { '' }
    Report '2. gcc 위치 = C:\Users\Public\vsm-gcc' ($first -ieq $LocalGcc) @("기대: $LocalGcc", "실제: $first")

    # 3. make clean · make - config.bat 없이 make.cmd 만 부른다
    if (Test-Path $Exe) { Remove-Item $Exe -Force }
    $build     = @(& cmd.exe /d /c ".\make.cmd clean >nul 2>&1 && .\make.cmd 2>&1")
    $buildExit = $LASTEXITCODE
    $oldTool   = @($build | Where-Object { "$_" -match 'Suresoft|CodeScroll' })
    $buildOk   = ($buildExit -eq 0) -and ($oldTool.Count -eq 0)
    if ($buildOk)
    {
        $warn = @($build | Where-Object { "$_" -match 'warning:' }).Count
        Report '3. make clean · make' $true @("exit 0 · 경고 $warn 건")
    }
    else
    {
        $detail = @("exit $buildExit") + @($oldTool | ForEach-Object { "옛 도구 경로: $_" }) + $build
        Report '3. make clean · make' $false $detail
    }

    # 4. 산출물
    Report '4. vsm_demo.exe 생성' (Test-Path $Exe) @($Exe)
}
finally
{
    Pop-Location
}

if ($script:failed -eq 0)
{
    Write-Output '결과: 통과 - 환경 세팅 · 빌드 성공'
    exit 0
}
Write-Output "결과: 실패 $($script:failed) 건 - setup_guide.md 「문제 해결」 참조"
exit 1
