"""Fast offline release-command authorization regression; no GitHub token."""
import unittest

from tools.release.parse_release_comment import parse

CMAKE = "project(SonkupikStudioNative VERSION 1.1.6 LANGUAGES CXX)\n"


class ReleaseCommentTests(unittest.TestCase):
    def valid(self, text):
        return parse(text, "masarray", "masarray", "241", False, CMAKE)

    def test_rc_and_stable(self):
        self.assertEqual(self.valid("/k500 release rc v1.1.6-rc.1"), ("rc", "1.1.6", "1"))
        self.assertEqual(self.valid("/k500 release rc v1.1.6-rc.12"), ("rc", "1.1.6", "12"))
        self.assertEqual(self.valid("/k500 release stable v1.1.6"), ("stable", "1.1.6", ""))

    def test_reject_unauthorized_and_wrong_issue(self):
        for actor, owner, issue, pr in (
            ("someone", "masarray", "241", False),
            ("masarray", "other", "241", False),
            ("masarray", "masarray", "240", False),
            ("masarray", "masarray", "241", True),
        ):
            with self.subTest(actor=actor, issue=issue, pr=pr):
                with self.assertRaises(ValueError):
                    parse("/k500 release rc v1.1.6-rc.1", actor, owner, issue, pr, CMAKE)

    def test_reject_wrong_version_old_rc_and_garbage(self):
        for comment in (
            "/k500 release rc v1.1.5-rc.2",
            "/k500 release stable v1.1.5",
            "/k500 release rc v1.1.6-rc.0",
            "/k500 release rc v1.1.6-rc.1; echo hacked",
            "/k500 release stable v1.1.6\n/k500 release rc v1.1.6-rc.2",
            "/k500 release rc v1.1.6-rc.1 ",
            " /k500 release rc v1.1.6-rc.1",
            "/k500 release rc v1.1.6-rc.10000",
            "",
        ):
            with self.subTest(comment=comment):
                with self.assertRaises(ValueError):
                    self.valid(comment)

    def test_reject_old_version_line_even_for_owner(self):
        old = "project(SonkupikStudioNative VERSION 1.1.5 LANGUAGES CXX)"
        with self.assertRaisesRegex(ValueError, "immutable"):
            parse("/k500 release stable v1.1.5", "masarray", "masarray", "241", False, old)


if __name__ == "__main__":
    unittest.main()
