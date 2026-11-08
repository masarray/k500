"""Offline K500 CI command authorization and smart-scope tests; no GitHub token."""
import unittest

from tools.ci.parse_ci_comment import parse_ci_comment
from tools.ci.change_scope import scopes


class CiCommandTests(unittest.TestCase):
    def test_owner_on_pr(self):
        for mode in ("fast", "smart", "windows", "installer", "platform", "full"):
            self.assertEqual(parse_ci_comment(
                "/k500 ci " + mode, "masarray", "masarray", "243", True
            ), mode)

    def test_issue_main(self):
        self.assertEqual(parse_ci_comment(
            "/k500 ci fast", "masarray", "masarray", "241", False
        ), "fast")

    def test_deny_untrusted(self):
        for actor, owner, number, is_pr in [
            ("attacker", "masarray", "241", False),
            ("masarray", "another", "241", False),
            ("masarray", "", "241", False),
            ("masarray", "masarray", "240", False),
            ("masarray", "masarray", "0", True),
            ("masarray", "masarray", "not-a-pr", True),
        ]:
            with self.assertRaises(ValueError):
                parse_ci_comment("/k500 ci full", actor, owner, number, is_pr)

    def test_deny_injected_or_replayed_text(self):
        for bad in (
            "/k500 ci full; echo pwned", "/k500 ci full\n/k500 release stable v1.1.6",
            " /k500 ci windows", "/k500 ci smart ", "/k500 ci FULL",
            "/k500 ci unknown", "", "/k500 release rc v1.1.6-rc.2",
        ):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                parse_ci_comment(bad, "masarray", "masarray", "241", False)


class CiScopeTests(unittest.TestCase):
    def test_auto_never_triggers_native(self):
        for mode in ("smart", "full", "windows", "installer", "platform", "fast"):
            self.assertEqual(scopes(mode, ["src/main.cpp", "<unknown-diff>"], False),
                             (False, False, False))

    def test_explicit_scope(self):
        self.assertEqual(scopes("fast", ["src/main.cpp"], True), (False, False, False))
        self.assertEqual(scopes("windows", [], True), (True, False, False))
        self.assertEqual(scopes("installer", [], True), (False, True, False))
        self.assertEqual(scopes("platform", [], True), (False, False, True))
        self.assertEqual(scopes("full", [], True), (True, True, True))

    def test_smart_path_routing(self):
        cases = [
            (["docs/README.md", "research/presets/trial.k500"], (False, False, False)),
            (["qml/components/Fader.qml"], (True, False, False)),
            (["packaging/windows/update_helper.cpp"], (True, True, False)),
            (["src/AppUpdateManager.cpp"], (True, True, False)),
            (["src/k500/K500PosixIo.cpp"], (False, False, True)),
            (["src/k500/K500TransactionScheduler.cpp"], (True, False, True)),
            (["CMakeLists.txt"], (True, True, True)),
            (["<unknown-diff>"], (True, True, True)),
        ]
        for paths, expected in cases:
            with self.subTest(paths=paths):
                self.assertEqual(scopes("smart", paths, True), expected)


if __name__ == "__main__":
    unittest.main()
