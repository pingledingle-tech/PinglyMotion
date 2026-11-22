#!/usr/bin/env python3
"""Builds the homepage: docs/index.html plus a live status section.

Used by .github/workflows/pages.yml. The section is built from GitHub's own
data (milestones, the owner test list issue, open pull requests), so nothing
is kept up to date by hand. Without data (no token, no network) the page is
still built, with a plain note instead of numbers.

Usage: python3 tools/site/build_status.py <out_dir>
Environment: GITHUB_TOKEN (optional), GITHUB_REPOSITORY (default below).
"""

import html
import json
import os
import re
import shutil
import sys
import urllib.request

REPO_DEFAULT = "pingledingle-tech/PinglyMotion"
TEST_LIST_ISSUE = 47
MARK_START = "<!--STATUS-->"
MARK_END = "<!--/STATUS-->"
MAX_MILESTONES = 3
MAX_BUILDING = 6
# files next to docs/index.html that are published with it
EXTRA_FILES = ["download.html", "privacy.html", "site.css", "site.js"]


def api(path, repo, token):
    req = urllib.request.Request("https://api.github.com/repos/%s/%s" % (repo, path))
    req.add_header("Accept", "application/vnd.github+json")
    req.add_header("User-Agent", "pinglymotion-site-build")
    if token:
        req.add_header("Authorization", "Bearer " + token)
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def fetch(repo, token):
    """Returns the raw data, or None when GitHub cannot be reached."""
    try:
        return {
            "milestones": api("milestones?state=open&per_page=20", repo, token),
            "test_list": api("issues/%d" % TEST_LIST_ISSUE, repo, token),
            "pulls": api("pulls?state=open&per_page=50", repo, token),
        }
    except Exception as e:  # network, rate limit, missing token: build without
        print("status data not available: %s" % e, file=sys.stderr)
        return None


def clean_title(title):
    """Titles shown on the page: no leading '[Draft]'-style tags or numbers."""
    return re.sub(r"\s*\(#\d+\)\s*$", "", title).strip()


def parse_waiting(body):
    """The tests on the owner test list: one per '### Test X' heading."""
    tests = []
    for m in re.finditer(r"^### Test \w+\s*\n+(.*?)(?=^### |\Z)", body or "", re.M | re.S):
        first = next((l.strip() for l in m.group(1).splitlines() if l.strip()), "")
        first = re.sub(r"^Pull request:\s*#\d+,?\s*", "", first)
        first = re.sub(r"^\(?|\)?$", "", first)
        # The first sentence is the plain name of the test.
        first = re.split(r"\.\s", first)[0].rstrip(".")
        if first:
            tests.append(first)
    return tests


def milestone_view(ms):
    done, left = ms.get("closed_issues", 0), ms.get("open_issues", 0)
    total = done + left
    pct = 0 if total == 0 else round(100 * done / total)
    if total == 0:
        words = "Nothing planned yet."
    elif left == 0:
        words = "All %d tasks done." % total
    else:
        words = "%d of %d tasks done, %d to go." % (done, total, left)
    return {"title": ms.get("title", ""), "words": words, "pct": pct,
            "url": ms.get("html_url", "")}


def own_pull(pull, repo):
    """True for a pull request from a branch of this repository, opened by its owner account.
    Pull requests from forks never put their title on the homepage (issue 252)."""
    head_repo = ((pull.get("head") or {}).get("repo") or {}).get("full_name")
    login = (pull.get("user") or {}).get("login")
    return head_repo == repo and login == repo.split("/")[0]


def build_view(data, repo=REPO_DEFAULT):
    """Plain structure that render() turns into HTML. Pure, so it is testable."""
    if data is None:
        return None
    ms = sorted(data["milestones"], key=lambda m: (m.get("due_on") or "9", m["number"]))
    own = [p for p in data["pulls"] if own_pull(p, repo)]
    pulls = [p for p in own if not p.get("draft")]
    drafts = [p for p in own if p.get("draft")]
    return {
        "milestones": [milestone_view(m) for m in ms[:MAX_MILESTONES]],
        "waiting": parse_waiting(data["test_list"].get("body")),
        "building": [clean_title(p["title"]) for p in drafts + pulls][:MAX_BUILDING],
    }


def li(items, empty):
    if not items:
        return "<p>%s</p>" % html.escape(empty)
    return "<ul>%s</ul>" % "".join("<li>%s</li>" % html.escape(i) for i in items)


def render(view):
    if view is None:
        body = ("<p class=\"desc\">The live status could not be loaded when this "
                "page was built. See the project on GitHub for the current state.</p>")
    else:
        if view["milestones"]:
            rows = "".join(
                "<div class=\"ms\"><h3>%s</h3><div class=\"bar\"><i style=\"width:%d%%\"></i></div>"
                "<p>%s</p></div>" % (html.escape(m["title"]), m["pct"], html.escape(m["words"]))
                for m in view["milestones"])
        else:
            rows = "<p class=\"desc\">No milestones are set up yet.</p>"
        body = ("<div class=\"ms-row\">%s</div><div class=\"two\">"
                "<div><h3>Waiting for the owner's test</h3>%s</div>"
                "<div><h3>Being built now</h3>%s</div></div>") % (
            rows,
            li(view["waiting"], "Nothing to test right now."),
            li(view["building"], "Nothing is being built right now."))
    return ("%s<section class=\"section\" id=\"status\"><div class=\"eyebrow\">Built in the open"
            "</div><h2>Project <em>status.</em></h2>%s"
            "<p class=\"note\">Built automatically from GitHub when the page is published.</p>"
            "</section>%s") % (MARK_START, body, MARK_END)


def insert(page, section):
    pattern = re.compile(re.escape(MARK_START) + ".*?" + re.escape(MARK_END), re.S)
    if not pattern.search(page):
        return page
    return pattern.sub(lambda _: section, page, count=1)


def main(argv):
    if len(argv) != 2:
        print(__doc__)
        return 2
    here = os.path.dirname(os.path.abspath(__file__))
    src = os.path.join(here, "..", "..", "docs", "index.html")
    repo = os.environ.get("GITHUB_REPOSITORY", REPO_DEFAULT)
    token = os.environ.get("GITHUB_TOKEN", "")
    with open(src, encoding="utf-8") as f:
        page = f.read()
    if MARK_START in page:
        out = insert(page, render(build_view(fetch(repo, token), repo)))
    else:
        out = page  # the page has no status section: publish it unchanged
    os.makedirs(argv[1], exist_ok=True)
    # the other pages and the shared style and script of the website
    for name in EXTRA_FILES:
        shutil.copyfile(os.path.join(here, "..", "..", "docs", name), os.path.join(argv[1], name))
    with open(os.path.join(argv[1], "index.html"), "w", encoding="utf-8") as f:
        f.write(out)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
