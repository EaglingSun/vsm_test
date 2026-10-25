"""check_junit.py 테스트 — tmp_path 에 JUnit XML 과 test_*.c 더미를 만들어 CLI 를 실제로 돌린다."""

import subprocess
import sys
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent / "check_junit.py"


def make_sources(tmp_path: Path, *suites: str) -> Path:
    """기대 스위트 폴더: test_<모듈>.c 더미 파일만 둔다."""
    src = tmp_path / "src"
    src.mkdir()
    for suite in suites:
        (src / f"{suite}.c").write_text("/* dummy */\n", encoding="utf-8")
    return src


def write_xml(results: Path, suite: str, cases: list[tuple[str, str | None]]) -> None:
    """vsm_test.h 와 같은 구조의 JUnit XML. cases = [(케이스, None=통과 | 이미 escape 된 실패 메시지)]."""
    results.mkdir(exist_ok=True)
    failures = sum(1 for _, msg in cases if msg is not None)
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        "<testsuites>",
        f'  <testsuite name="{suite}" tests="{len(cases)}" failures="{failures}" errors="0">',
    ]
    for name, msg in cases:
        if msg is None:
            lines.append(f'    <testcase classname="{suite}" name="{name}"/>')
        else:
            lines.append(f'    <testcase classname="{suite}" name="{name}">')
            lines.append(f'      <failure message="{msg}">{msg} (failed checks: 1)</failure>')
            lines.append("    </testcase>")
    lines += ["  </testsuite>", "</testsuites>", ""]
    (results / f"{suite}.xml").write_text("\n".join(lines), encoding="utf-8")


def write_expect(tmp_path: Path, *lines: str) -> Path:
    path = tmp_path / "expected_failures.txt"
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return path


def run(src: Path, results: Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        [sys.executable, str(SCRIPT), str(results), "--sources", str(src), *args],
        capture_output=True,
        text=True,
        encoding="utf-8",
        timeout=60,
    )


def test_expect_matches_exit0(tmp_path):
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    write_xml(results, "test_a", [("SR-001_ok", None), ("SR-002_bad", "x.c:1: a == b")])
    expect = write_expect(tmp_path, "# 주석 줄", "", "test_a::SR-002_bad")
    r = run(src, results, "--expect", str(expect))
    assert r.returncode == 0, r.stdout + r.stderr
    assert "통과 1" in r.stdout
    assert "알려진 실패 1" in r.stdout


def test_unexpected_failure_exit1(tmp_path):
    """ⓐ 실패했는데 목록에 없음."""
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    write_xml(results, "test_a", [("SR-001_ok", None), ("SR-002_bad", "x.c:1: a == b")])
    expect = write_expect(tmp_path, "# 비어 있음")
    r = run(src, results, "--expect", str(expect))
    assert r.returncode == 1
    assert "test_a::SR-002_bad" in r.stdout


def test_expected_failure_passed_exit1(tmp_path):
    """ⓑ 목록에 있는데 통과함."""
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    write_xml(results, "test_a", [("SR-001_ok", None)])
    expect = write_expect(tmp_path, "test_a::SR-001_ok")
    r = run(src, results, "--expect", str(expect))
    assert r.returncode == 1
    assert "test_a::SR-001_ok" in r.stdout


def test_expected_case_missing_exit1(tmp_path):
    """ⓒ 목록에 있는데 결과에 그런 케이스가 없음."""
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    write_xml(results, "test_a", [("SR-001_ok", None)])
    expect = write_expect(tmp_path, "test_a::SR-999_gone")
    r = run(src, results, "--expect", str(expect))
    assert r.returncode == 1
    assert "test_a::SR-999_gone" in r.stdout


def test_no_expect_exit_by_failures(tmp_path):
    """--expect 없음: 실패(<failure> · <error>)가 있으면 1, 없으면 0."""
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    write_xml(results, "test_a", [("SR-001_ok", None)])
    assert run(src, results).returncode == 0

    (results / "test_a.xml").write_text(
        '<?xml version="1.0" encoding="UTF-8"?>\n<testsuites>'
        '<testsuite name="test_a" tests="1" failures="0" errors="1">'
        '<testcase classname="test_a" name="SR-001_err"><error message="boom"/></testcase>'
        "</testsuite></testsuites>\n",
        encoding="utf-8",
    )
    r = run(src, results)
    assert r.returncode == 1
    assert "test_a::SR-001_err" in r.stdout


def test_missing_xml_exit2(tmp_path):
    src = make_sources(tmp_path, "test_a", "test_b")
    results = tmp_path / "results"
    write_xml(results, "test_a", [("SR-001_ok", None)])
    r = run(src, results, "--expect", str(write_expect(tmp_path, "# 비어 있음")))
    assert r.returncode == 2
    assert "test_b.xml" in r.stdout + r.stderr


def test_unparsable_xml_exit2(tmp_path):
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    results.mkdir()
    (results / "test_a.xml").write_text("<testsuites><testsuite name=", encoding="utf-8")
    r = run(src, results)
    assert r.returncode == 2
    assert "test_a.xml" in r.stdout + r.stderr


def test_zero_testcases_exit2(tmp_path):
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    write_xml(results, "test_a", [])
    r = run(src, results)
    assert r.returncode == 2
    assert "test_a.xml" in r.stdout + r.stderr


def test_no_sources_exit2(tmp_path):
    """기대 스위트(test_*.c)가 하나도 없으면 판정할 것이 없다 — 통과가 아니라 오류."""
    src = make_sources(tmp_path)
    results = tmp_path / "results"
    results.mkdir()
    assert run(src, results).returncode == 2


def test_summary_appends_table(tmp_path):
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    write_xml(results, "test_a", [("SR-001_ok", None), ("SR-002_bad", "x.c:1: a == b")])
    summary = tmp_path / "summary.md"
    summary.write_text("기존 내용\n", encoding="utf-8")

    r = run(src, results, "--summary", str(summary))
    assert r.returncode == 1  # 판정 결과와 관계없이 요약은 쓴다
    run(src, results, "--summary", str(summary))

    text = summary.read_text(encoding="utf-8")
    assert text.startswith("기존 내용\n")
    assert text.count("| 스위트 | 케이스 | 결과 | 메시지 |") == 2
    assert "| test_a | SR-001_ok | 통과 |" in text
    assert "| test_a | SR-002_bad | 실패 |" in text
    assert "x.c:1: a == b" in text


def test_escaped_message_is_unescaped(tmp_path):
    src = make_sources(tmp_path, "test_a")
    results = tmp_path / "results"
    write_xml(results, "test_a", [("SR-002_bad", "x.c:7: a &lt; b &amp;&amp; c &gt; &quot;d&quot;")])
    summary = tmp_path / "summary.md"
    r = run(src, results, "--summary", str(summary))
    assert r.returncode == 1
    assert 'x.c:7: a < b && c > "d"' in r.stdout
    assert 'x.c:7: a < b && c > "d"' in summary.read_text(encoding="utf-8")
