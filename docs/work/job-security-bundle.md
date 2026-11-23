Branch: job/security-bundle
Status: active
Pull request: pending
Files:
- CLAUDE.md
- docs/DEV.md
- .github/workflows/codeql.yml
- docs/SECURITY_PRIVACY.md
- docs/TEACHING.md
- docs/README.md
- SECURITY.md

## What

Security bundle (lane 2). Four parts: a standing rule in CLAUDE.md for reading
and fixing security alerts inside the existing job queue; a step in the nightly
CodeQL run that lists open code scanning alerts in the run summary; the AI
notice text (job K), checked against the code first; and a security and privacy
page in docs/ with only proven claims (job L2-security).
