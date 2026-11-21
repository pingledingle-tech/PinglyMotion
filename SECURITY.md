# Security policy

## Supported versions

PinglyMotion is a small hobby project. Security fixes are made on the `main`
branch and go into the next release. Only the latest release gets fixes.

| Version | Supported |
|---|---|
| Latest release (the next one is 0.3.x) | Yes |
| Older releases | No |
| QAvimator (the program PinglyMotion is based on) | No – it is no longer maintained |

## Reporting a vulnerability

Please report security problems **privately**, not in a public issue:

1. Open the **Security** tab of this repository.
2. Choose **Report a vulnerability**
   ([direct link](https://github.com/pingledingle-tech/PinglyMotion/security/advisories/new)).

Please include:

- what the problem is and which part of PinglyMotion it affects (for example
  opening a file, remote control, the MCP server or the Claude Desktop
  extension, the live link to Aurora, the Kimodo helper, or the build and
  release workflows)
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
  PinglyMotion opens or imports: `.dingle`, `.bvh`, `.avm`, Second Life
  `.anim`, glTF (`.glb`, `.gltf`), `.prp` props, joint mapping files, the
  reference library and data files
- the remote control interface (`PinglyMotion --control`, local HTTP on
  127.0.0.1, with a key), the MCP servers in `tools/mcp/` and the Claude
  Desktop extension (`PinglyMotion.mcpb`)
- the live link to the Aurora viewer, including its mutual authentication
- the optional Kimodo text-to-motion helper in `tools/kimodo/` (its README
  says which versions are pinned and how)
- the update check (it only reads the newest release from GitHub)
- the GitHub Actions workflows, the combined test build script and the
  release packages

Out of scope:

- problems that need an attacker to already run code as the same user on the
  same computer, unless PinglyMotion makes the attack noticeably easier
- vulnerabilities in Qt, Second Life viewers or other third-party software –
  please report those to their own projects (but do tell us if PinglyMotion ships
  an affected version)

## Verifying a download

Each release has a `SHA256SUMS` file and a signed build provenance
attestation, which shows that the zip was built by this repository's release
workflow from a given commit.

    sha256sum --check SHA256SUMS
    gh attestation verify PinglyMotion-vX.Y.Z-windows.zip --repo pingledingle-tech/PinglyMotion

On Windows, `Get-FileHash PinglyMotion-vX.Y.Z-windows.zip` shows the SHA-256
to compare with the one in `SHA256SUMS`.

## Known issues

The security audits are in [docs/SECURITY_AUDIT.md](docs/SECURITY_AUDIT.md)
(November 2026) and [docs/SECURITY_AUDIT_21NOV.md](docs/SECURITY_AUDIT_21NOV.md)
(21 November 2026). Their findings are public issues labelled
[`security`](https://github.com/pingledingle-tech/PinglyMotion/issues?q=label%3Asecurity)
and
[`security-hardening`](https://github.com/pingledingle-tech/PinglyMotion/issues?q=label%3Asecurity-hardening);
they do not need to be reported again.

