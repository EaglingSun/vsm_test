#!/usr/bin/env python3
"""
JUnit XML 판정 · 알려진 실패 대조 · 요약 표 (ADR-002 · ADR-003)

Usage:
    python tests/check_junit.py RESULTS_DIR [--expect FILE] [--summary FILE] [--sources DIR]

Exit code: 0 = 통과, 1 = 판정 실패, 2 = 결과 파일 오류(누락 · 파싱 실패 · testcase 0개) 또는 사용 오류

판정 방식
    기대 스위트 = --sources 폴더(기본: 이 스크립트 폴더)의 test_*.c 파일 이름.
    각 스위트마다 RESULTS_DIR/<이름>.xml 이 있어야 하고, 파싱되고, testcase 가 1개 이상이어야 한다.
    결과 식별자 = <testsuite name>::<testcase name>. <failure> 또는 <error> 가 있으면 실패.

    --expect 없음: 실패가 있으면 exit 1.
    --expect FILE: 빈 줄 · # 줄을 뺀 한 줄 = 식별자 하나. 아래 중 하나라도 있으면 exit 1.
        ⓐ 실패했는데 목록에 없음  ⓑ 목록에 있는데 통과함  ⓒ 목록에 있는데 결과에 그런 케이스가 없음
    --summary FILE: 마크다운 표(스위트 · 케이스 · 결과 · 메시지)와 합계를 덧붙인다. 판정 결과와 관계없이 쓴다.

주의
    결과 파일 누락 · 손상을 "실패 0건" 으로 읽지 않는다. 그런 경우는 언제나 exit 2 다.
"""

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent


class Result:
    """testcase 하나의 판정 결과."""

    def __init__(self, suite: str, case: str, message: str | None):
        self.suite = suite
        self.case = case
        self.message = message  # None = 통과

    @property
    def ident(self) -> str:
        return f"{self.suite}::{self.case}"

    @property
    def failed(self) -> bool:
        return self.message is not None


def expected_suites(sources: Path) -> list[str]:
    return sorted(p.stem for p in sources.glob("test_*.c"))


def parse_xml(path: Path) -> list[Result]:
    """XML 한 개를 읽는다. 누락 · 파싱 실패 · 이름 없는 요소 · testcase 0개는 ValueError."""
    if not path.is_file():
        raise ValueError(f"{path}: 결과 파일이 없습니다")
    try:
        root = ET.parse(path).getroot()
    except (ET.ParseError, OSError) as e:
        raise ValueError(f"{path}: XML 을 읽지 못했습니다 — {e}") from e

    results = []
    for ts in root.iter("testsuite"):
        suite = ts.get("name")
        if not suite:
            raise ValueError(f"{path}: name 이 없는 testsuite 가 있습니다")
        for tc in ts.iter("testcase"):
            case = tc.get("name")
            if not case:
                raise ValueError(f"{path}: name 이 없는 testcase 가 있습니다 (스위트 {suite})")
            bad = tc.find("failure")
            if bad is None:
                bad = tc.find("error")
            message = None
            if bad is not None:
                message = bad.get("message") or (bad.text or "").strip() or bad.tag
            results.append(Result(suite, case, message))
    if not results:
        raise ValueError(f"{path}: testcase 가 하나도 없습니다")
    return results


def load_results(results_dir: Path, suites: list[str]) -> tuple[list[Result], list[str]]:
    """기대 스위트의 XML 을 모두 읽는다. 반환 = (결과, 오류 목록)."""
    if not suites:
        return [], ["기대 스위트(test_*.c)가 하나도 없습니다 — 판정할 것이 없습니다"]
    results, errors = [], []
    for suite in suites:
        try:
            results.extend(parse_xml(results_dir / f"{suite}.xml"))
        except ValueError as e:
            errors.append(str(e))
    return results, errors


def load_expect(path: Path) -> set[str]:
    lines = path.read_text(encoding="utf-8").splitlines()
    return {s for s in (line.strip() for line in lines) if s and not s.startswith("#")}


def md_cell(text: str) -> str:
    """표 칸 하나: 줄바꿈 · | 가 표를 깨지 않게, < 가 HTML 로 읽히지 않게 코드 span 으로 감싼다."""
    text = " ".join(text.split()).replace("`", "'").replace("|", "\\|")
    return f"`{text}`" if text else ""


def write_summary(path: Path, results: list[Result], expect: set[str] | None, errors: list[str]) -> None:
    out = ["", "## VSM 테스트 결과", ""]
    if errors:
        out.append("결과 파일 오류 — 판정할 수 없습니다:")
        out += [f"- {md_cell(e)}" for e in errors]
    else:
        out += ["| 스위트 | 케이스 | 결과 | 메시지 |", "|---|---|---|---|"]
        for r in results:
            if not r.failed:
                verdict = "통과"
            elif expect is not None and r.ident in expect:
                verdict = "실패 (알려진 실패)"
            else:
                verdict = "실패"
            out.append(f"| {r.suite} | {r.case} | {verdict} | {md_cell(r.message or '')} |")
        failed = sum(1 for r in results if r.failed)
        out += ["", f"합계: 테스트 {len(results)} · 통과 {len(results) - failed} · 실패 {failed}"]
    with path.open("a", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")


def judge(results: list[Result], expect: set[str] | None) -> int:
    """판정 결과를 출력하고 exit 코드(0 · 1)를 돌려준다."""
    failed = [r for r in results if r.failed]
    passed = len(results) - len(failed)

    if expect is None:
        for r in failed:
            print(f"실패: {r.ident} — {r.message}")
        print(f"통과 {passed} · 실패 {len(failed)}")
        return 1 if failed else 0

    all_ids = {r.ident for r in results}
    failed_ids = {r.ident for r in failed}
    unexpected = [r for r in failed if r.ident not in expect]                    # ⓐ
    now_passing = sorted((expect & all_ids) - failed_ids)                        # ⓑ
    not_found = sorted(expect - all_ids)                                         # ⓒ

    if unexpected:
        print("ⓐ 실패했는데 알려진 실패 목록에 없음:")
        for r in unexpected:
            print(f"  - {r.ident} — {r.message}")
    if now_passing:
        print("ⓑ 알려진 실패 목록에 있는데 통과함 (고쳤으면 목록에서 지운다):")
        for ident in now_passing:
            print(f"  - {ident}")
    if not_found:
        print("ⓒ 알려진 실패 목록에 있는데 결과에 그런 케이스가 없음:")
        for ident in not_found:
            print(f"  - {ident}")
    if unexpected or now_passing or not_found:
        return 1
    print(f"통과 {passed} · 알려진 실패 {len(failed)}")
    return 0


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

    p = argparse.ArgumentParser(description="JUnit XML 판정 · 알려진 실패 대조 · 요약 표")
    p.add_argument("results_dir", type=Path)
    p.add_argument("--expect", type=Path, help="알려진 실패 목록 (<스위트>::<케이스> 한 줄에 하나)")
    p.add_argument("--summary", type=Path, help="마크다운 요약 표를 덧붙일 파일")
    p.add_argument("--sources", type=Path, default=HERE, help="기대 스위트(test_*.c) 폴더 (기본: 이 스크립트 폴더)")
    args = p.parse_args()

    expect = None
    if args.expect is not None:
        try:
            expect = load_expect(args.expect)
        except OSError as e:
            print(f"ERROR: 알려진 실패 목록을 읽지 못했습니다 — {e}", file=sys.stderr)
            return 2

    results, errors = load_results(args.results_dir, expected_suites(args.sources))
    if args.summary is not None:
        write_summary(args.summary, results, expect, errors)
    if errors:
        print("ERROR: 결과 파일 오류 — 판정할 수 없습니다:", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        return 2
    return judge(results, expect)


if __name__ == "__main__":
    sys.exit(main())
