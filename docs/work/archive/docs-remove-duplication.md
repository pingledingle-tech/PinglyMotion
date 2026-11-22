Branch: docs/remove-duplication
Status: done
Pull request: #332
Files:
- CLAUDE.md
- docs/DEV.md
- docs/PRODUCT.md
- docs/POSING_UX.md
- docs/UX_DECISIONS.md
- docs/TEACHING_TEXTS.md
- docs/TEACHING.md
- docs/ASSISTANT_GUIDE.md
- docs/EASING.md

## What

Docs only. Remove duplicated text: CLAUDE.md is the single source for the
process rules (DEV.md keeps only the how-to), one UI step numbering with a
mapping table for old numbers, one glossary, a purpose line on each Script Lab
document, and "When you are done" step 2 uses the GitHub "Update branch" API
instead of a rebase so own branches never need a force-push. Approved by the
project owner on 22 Nov 2026.

Left out: the purpose line on the Script Lab documents (SCRIPT_LAB.md,
SCRIPT_LAB_PLAN.md, CONTROL.md, ROADMAP.md), because open Script Lab pull
requests change those files. docs/WORK.md stays a pointer page (MIGRATION.md
links to it).
