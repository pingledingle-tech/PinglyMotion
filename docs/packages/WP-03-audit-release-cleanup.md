ID: WP-03
Title: Audit Follow-ups, Release Polish, and Code Cleanup
Status: waiting
Owner-approved: no
Priority: 3
Reason: Security audit R0-R8 completed with no critical/high findings; remaining items wait for owner approval.

## Tasks

- [ ] Bundle third-party licence notices in Windows zip and AppImage, unify GPL-2.0-or-later statement, add unsigned-binary notice on download page, and fix minor README/NEWS wording (audit J03-J08)
- [ ] Add controller/contact details to privacy page (needs owner input)
- [ ] Implement signed automatic client update (`ed25519` + SHA-256 verification before file swap)
- [ ] Standardize error-reporting convention across new and old code (issue #207)
- [ ] Remove temporary QAvimator scene-refactor scaffolding (issue #200)
