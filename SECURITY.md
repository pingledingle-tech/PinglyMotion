# Security policy

## Supported versions

Pingmator is a small hobby project. Security fixes are made on the `main`
branch and go into the next release. Only the latest release gets fixes.

| Version | Supported |
|---|---|
| Latest release (currently 0.2.x) | Yes |
| Older releases | No |
| QAvimator (the program Pingmator is based on) | No – it is no longer maintained |

## Reporting a vulnerability

Please report security problems **privately**, not in a public issue:

1. Open the **Security** tab of this repository.
2. Choose **Report a vulnerability**
   ([direct link](https://github.com/pingledingle-tech/PinglyMotion/security/advisories/new)).

Please include:

- what the problem is and which part of Pingmator it affects (for example
  opening a BVH/AVM file, remote control, the MCP server, the live link to
  Aurora, or the build and release workflows)
- the version or commit you tested
- the steps or a small file that shows the problem
- what an attacker could do with it

You will get an answer when the report has been read. This is a spare-time
project, so there is no fixed response time. Reports are handled privately
until a fix is available; after that the advisory is published, with credit
if you want it.

## Scope

In scope:

- crashes, memory errors, hangs or excessive memory use caused by files
  Pingmator opens (`.bvh`, `.avm`, `.prp`, data files)
- the remote control interface (`pingmator --control`, local HTTP on
  127.0.0.1) and the MCP server in `tools/mcp/`
- the live link to the Aurora viewer
- the GitHub Actions workflows and the release packages

Out of scope:

- problems that need an attacker to already run code as the same user on the
  same computer, unless Pingmator makes the attack noticeably easier
- vulnerabilities in Qt, Second Life viewers or other third-party software –
  please report those to their own projects (but do tell us if Pingmator ships
  an affected version)

## Verifying a download

Each release has a `SHA256SUMS` file and a signed build provenance
attestation, which shows that the zip was built by this repository's release
workflow from a given commit.

    sha256sum --check SHA256SUMS
    gh attestation verify Pingmator-vX.Y.Z-windows.zip --repo pingledingle-tech/PinglyMotion

On Windows, `Get-FileHash Pingmator-vX.Y.Z-windows.zip` shows the SHA-256
to compare with the one in `SHA256SUMS`.

## Known issues

The security audit of November 2026 is in
[docs/SECURITY_AUDIT.md](docs/SECURITY_AUDIT.md). Its findings are public
issues labelled
[`security`](https://github.com/pingledingle-tech/PinglyMotion/issues?q=label%3Asecurity)
and
[`security-hardening`](https://github.com/pingledingle-tech/PinglyMotion/issues?q=label%3Asecurity-hardening);
they do not need to be reported again.

