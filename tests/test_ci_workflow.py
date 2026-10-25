"""CI 워크플로(.github/workflows/test.yml) 구조 검사 — ADR-003.

YAML 구조만 본다. GitHub 에서 실제로 도는지는 PR 실행으로만 확인된다.
action SHA 의 원천은 docs/ADR.md, gcc 캐시 키의 원천은 setup/setup_gcc.ps1 이다 — 여기에 값을 다시 적지 않는다.
"""

import re
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parent.parent
WORKFLOW = ROOT / ".github" / "workflows" / "test.yml"
ADR = ROOT / "docs" / "ADR.md"
SETUP_GCC = ROOT / "setup" / "setup_gcc.ps1"

PINNED_USES = re.compile(
    r"actions/(?:checkout|cache/restore|cache/save|upload-artifact)@[0-9a-f]{40}(?![0-9a-f])"
)


@pytest.fixture(scope="module")
def wf() -> dict:
    return yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))


@pytest.fixture(scope="module")
def job(wf) -> dict:
    return wf["jobs"]["test"]


@pytest.fixture(scope="module")
def steps(job) -> list[dict]:
    return job["steps"]


def find_step(steps: list[dict], pred) -> dict:
    found = [s for s in steps if pred(s)]
    assert len(found) == 1, f"조건에 맞는 step 이 1개가 아니라 {len(found)}개"
    return found[0]


def uses_step(steps: list[dict], action: str) -> dict:
    return find_step(steps, lambda s: s.get("uses", "").startswith(f"actions/{action}@"))


def test_triggers_pull_request_master_and_dispatch(wf):
    on = wf.get("on", wf.get(True))  # PyYAML 은 키 on 을 불리언 True 로 읽는다
    assert "master" in on["pull_request"]["branches"]
    assert "workflow_dispatch" in on


def test_no_pull_request_target_or_secrets():
    text = WORKFLOW.read_text(encoding="utf-8")
    assert "pull_request_target" not in text
    assert "secrets" not in text


def test_permissions_contents_read_only(wf, job):
    assert wf["permissions"] == {"contents": "read"}
    assert "permissions" not in job  # job 수준에서 넓히지 않는다


def test_runs_on_windows_latest(job):
    assert job["runs-on"] == "windows-latest"


def test_uses_pinned_to_adr_sha(wf):
    uses = [s["uses"] for j in wf["jobs"].values() for s in j.get("steps", []) if "uses" in s]
    for u in uses:
        assert PINNED_USES.fullmatch(u), f"공식 action + 40자 커밋 SHA 가 아님: {u}"
    adr = set(PINNED_USES.findall(ADR.read_text(encoding="utf-8")))
    assert adr, "docs/ADR.md 에서 actions/...@<sha> 를 하나도 찾지 못했다"
    assert set(uses) == adr


def make_test_step(steps: list[dict]) -> dict:
    return find_step(steps, lambda s: s.get("run", "").strip() == r".\make.cmd test")


@pytest.mark.parametrize("action", ["cache/restore", "cache/save"])
def test_cache_key_has_setup_gcc_sha256(steps, action):
    m = re.search(r"^\$Sha256\s*=\s*'([0-9A-Fa-f]{64})'", SETUP_GCC.read_text(encoding="utf-8"), re.M)
    assert m, "setup/setup_gcc.ps1 에서 $Sha256 값을 찾지 못했다"
    key = uses_step(steps, action)["with"]["key"]
    assert m.group(1).lower() in key.lower()


def test_no_combined_cache_action():
    # actions/cache@ 한 step 은 job 이 성공해야 저장한다(post-if: success()) — 결함판 PR 에서는 저장되지 않는다
    assert "actions/cache@" not in WORKFLOW.read_text(encoding="utf-8")


def test_cache_save_before_make_test(steps):
    # 테스트가 실패해도 캐시는 이미 저장돼 있어야 한다
    save = uses_step(steps, "cache/save")
    assert steps.index(save) < steps.index(make_test_step(steps))


def test_cache_save_only_on_cache_miss(steps):
    # 같은 키가 이미 있으면 저장이 실패로 끝난다 — 복원 step 이 못 찾았을 때만 저장한다
    restore_id = uses_step(steps, "cache/restore").get("id")
    assert restore_id, "캐시 복원 step 에 id 가 없다"
    cond = uses_step(steps, "cache/save").get("if", "")
    assert f"steps.{restore_id}.outputs.cache-hit" in cond
    assert "'true'" in cond


def test_runs_make_test(steps):
    make_test_step(steps)


def test_summary_step_runs_even_after_failure(steps):
    step = find_step(steps, lambda s: "check_junit.py" in s.get("run", ""))
    assert "check_junit.py test-results --summary" in step["run"]
    assert "--expect" not in step["run"]  # CI 는 실제 결과를 보여 준다 — 알려진 실패 대조는 로컬용
    assert "!cancelled()" in step.get("if", "")


def test_artifact_step_uploads_xml_or_errors(steps):
    step = uses_step(steps, "upload-artifact")
    assert "!cancelled()" in step.get("if", "")
    assert step["with"]["path"] == "test-results/*.xml"
    assert step["with"]["if-no-files-found"] == "error"
