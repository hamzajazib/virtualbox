# AGENTS.md — VirtualBox (`trunk/`)

This file defines how coding agents should work in the VirtualBox source tree.
It applies to both the internal SVN repository and the public GitHub mirror.

If a more specific `AGENTS.md` exists in a subdirectory, follow the nearest one.
Also follow repo-local coding-guideline documents referenced below; several
subtrees have stricter local rules than the tree-wide defaults.

## 1. Repository Model

VirtualBox development is centered on SVN:

- The canonical repository is SVN.
- The GitHub repository is a public mirror and may lag SVN.
- Oracle-internal work always lands in SVN.
- External contributors typically collaborate through GitHub issues and pull requests.

Do not assume GitHub reflects the newest internal state when investigating
regressions or deciding whether a problem is already fixed.

Assume git commands will not work. Always start with svn.

## 2. Default Agent Workflow

Agents should normally:

1. Inspect the relevant code and local documentation before editing.
2. Make the smallest change that fixes the issue safely.
3. Keep changes close to the affected subsystem.
4. Build and test the narrowest relevant targets when practical.
5. Report exactly what was changed, what was run, and what remains unverified.

Do not:

- create commits, branches, tags, or pushes unless explicitly asked by the user
- rewrite history
- make opportunistic refactors unrelated to the task
- edit fetched toolchains, vendored code, or build-system internals unless the task requires it
- edit generated outputs, disk images, bugreports, logs, or `out/` artifacts unless the task is specifically about them

## 3. Tree Layout

Important top-level locations under `trunk/`:

- `.github/` — mirror-facing GitHub workflows and templates
- `debian/` — Debian packaging
- `doc/` — coding guidelines, manual sources, and technical documentation
- `include/` — public/internal headers, including IPRT headers
- `src/` — product and IPRT source code
- `tools/` — development tools and environment setup
- `webtools/` — internal/supporting web tooling
- `configure.py` — external-developer environment setup
- `LocalConfig.kmk` — local, machine-specific configuration; do not commit user-local tweaks
- `out/` — build outputs; do not hand-edit

Additionally, these are the important locations under `trunk/src/`:
- `libs/` - Most 3rd party libraries currently in use in VirtualBox code.
- `VBox/Additions` - Guest aditions, this gets packaged up and installed by the end-user on VMs running in VirtualBox
- `VBox/Debugger` - Built-in VM debugger (examine and control low level VM state).
- `VBox/Devices` - Various guest devices that the VMM virtualizes, such as the audio, network, and graphics card, as well as the virtual hard disk and USB controller.
- `VBox/Disassembler` - Disassembler which analyzes guest code on commission of various other VirtualBox components
- `VBox/Frontends` - Various user interfaces. The primary two being `VBox/Frontends/VirtualBox` (the GUI that most end-users will use) and `VBox/Frontends/VBoxManage` the textual interface that exposes the API in Main to the command line
- `VBox/GuestHost` - Shared protocol/data/path/MIME/transfer logic reused by host services and Guest Additions.
- `VBox/HostDrivers` - All things related drivers that need to be installed on the end-user host machine for proper functionality.
- `VBox/HostServices` - Features that bridge the host and guest (VM) such as shared clipboard, shared folders, drag and drop of files, etc.
- `VBox/Installer` - Installers for all supported host OSes
- `VBox/Main` - The XPCOM backend API which lies on top of the VMM code. All frontends utilize this Main API to control VirtualBox.
- `VBox/NetworkServices` - Network services for VirtualBox including Internal Networking, DHCP, and (parts of) NAT Network.
- `VBox/Runtime` - Contains IPRT: A portable runtime library which abstracts file access, threading, and string manipulation.
- `VBox/Storage` - Support for different virtual storage types
- `VBox/ValidationKit` - Automated testing suite for VirtualBox
- `VBox/VMM` - The Virtual Machine Monitor. Core of the hypervisor.

Third-party or imported code lives primarily under `src/libs/` and some firmware
subtrees. In those areas, preserve the local style and keep the VirtualBox delta
small and well-isolated.

## 4. Environment Setup

### 4.1 Internal Oracle developers

Internal development typically uses:

- `tools/env.sh` on Unix-like hosts
- `tools/env.cmd` / `tools/win64env.cmd` on Windows
- `kmk -C tools fetch` to fetch compilers and build tools

`tools/env.sh` is not just a convenience wrapper; it establishes the expected
developer shell for this tree. Durable behavior worth knowing:

- it derives the repo root from `tools/env.sh` and normally changes into that root
- it exports the modern `KBUILD_*` variables and treats old `BUILD_*` names as obsolete
- it defaults `KBUILD_HOST` / `KBUILD_TARGET` to the current host unless explicitly overridden
- it defaults `KBUILD_TYPE` to `debug`
- recognized build types include `release`, `profile`, `kprofile`, `debug`, `strict`, `dbgopt`, and `asan`
- it defaults `KBUILD_DEVTOOLS` to `trunk/tools`
- it defaults `KBUILD_PATH` to `trunk/kBuild`
- it prepends the appropriate tool and output directories to `PATH`, so built binaries and tools are found from the shell
- on WSL it exports `VBOX_IN_WSL=1` and `KBUILD_NO_HARD_LINKING=1`
- if invoked without a command it spawns a work shell; if invoked with a command it executes that command in the prepared environment

Agents should prefer the `KBUILD_*` variable names in documentation and command
examples and should not introduce new guidance based on obsolete `BUILD_*`
variables.

Do not use `configure.py` for the internal workflow.

### 4.2 External developers

External developers typically use `configure.py` and then build with `kmk`.
`README.md` points to the canonical external build instructions page and the
developer documentation entry points.

A practical external setup flow is:

1. read `README.md` and the linked build-instructions page for host-specific prerequisites
2. run `configure.py`
3. review or create `LocalConfig.kmk` for machine-local overrides
4. build with `kmk`, or `kmk -C <subdir>` for a narrower target

Useful external-developer notes:

- `configure.py` is the expected entry point for setting up an external build environment
- `LocalConfig.kmk` is the right place for local-only toggles, fetched-tool locations, and developer convenience knobs
- `doc/kBuild-tricks.txt` includes additional kBuild and `LocalConfig.kmk` tips, including `FETCHDIR`
- external builds still use the same `KBUILD_*` vocabulary as the internal environment, so `KBUILD_TYPE=debug` / `release` and related variables are the right knobs to document
- if testcase binaries are needed, enable them locally with `VBOX_WITH_TESTCASES=1`
- if Validation Kit artifacts are needed, enable them locally with `VBOX_WITH_VALIDATIONKIT=1`
- if local hardening gets in the way of development/debugging, `VBOX_WITHOUT_HARDENING=1` is a local-only workaround, not a source change

Agents should treat external environment setup as host-specific and avoid
guessing missing system packages or SDK details when the linked build
instructions should be consulted instead.

### 4.3 Local configuration

`LocalConfig.kmk` is for machine-local settings only. Common local knobs include:

- `VBOX_WITHOUT_HARDENING=1`
- `VBOX_WITH_VALIDATIONKIT=1`
- `VBOX_WITH_TESTCASES=1`

Agents may suggest local `LocalConfig.kmk` changes for development or testing,
but should not treat them as source changes to commit.

## 5. Build And Test Expectations

The default build is usually `debug`, with outputs under:

- `out/<host>.<arch>/<build-type>/`

Common build patterns:

- whole tree: `kmk`
- subtree: `kmk -C <subdir>`
- clean subtree: `kmk -C <subdir> clean`

Environment notes derived from `tools/env.sh`:

- absent an override, assume `KBUILD_TYPE=debug`
- prefer documenting `KBUILD_*` variables over obsolete `BUILD_*` names
- internal `env.sh` shells usually have the relevant `out/.../bin` and `out/.../bin/tools` directories on `PATH`

Useful test/build entry points already in the tree:

- IPRT / Runtime testcases: `src/VBox/Runtime/testcase/`
- Storage testcases: `src/VBox/Storage/testcase/`
- Validation Kit sources: `src/VBox/ValidationKit/`
- Validation Kit ISO target: `validationkit-iso` in `src/VBox/ValidationKit/Makefile.kmk`

Typical narrow builds:

- `kmk -C src/VBox/Runtime/testcase`
- `kmk -C src/VBox/Storage/testcase`
- `kmk -C src/VBox/ValidationKit validationkit-iso`

Notes:

- Many tests require `VBOX_WITH_TESTCASES=1`.
- Validation Kit packaging and some unit-test packing depend on Validation Kit config.
- Validation Kit guest images and TXS setup are documented in:
  - `src/VBox/ValidationKit/readme.txt`
  - `src/VBox/ValidationKit/vms/readme_first.txt`

When a full validation pass is not practical, provide a concrete manual test
plan and call out missing coverage explicitly, especially for changes affecting:

- host drivers
- hardening
- networking
- storage
- HGCM / host services
- Guest Additions
- cross-host or cross-guest behavior

## 6. Coding Style Rules

The authoritative tree-wide coding guide is `doc/VBox-CodingGuidelines.cpp`.
Agents must follow it. Key mandatory rules include:

- Use 4-space indentation.
- Tabs are only for makefiles.
- Prefer RT/VBOX types and runtime helpers.
- Standard `bool`, `uintptr_t`, `intptr_t`, and fixed-width integer types are acceptable and preferred over portability-hostile plain `int`, `unsigned`, or `long`.
- Use `uintptr_t` / `intptr_t` for pointer-to-integer casts and pointer arithmetic.
- Use `RT_OS_*` macros, not compiler-provided OS macros such as `_WIN32`.
- Treat `char *` strings as UTF-8, but do not trust external strings to actually be valid UTF-8.
- Use `static` for internal symbols wherever possible.
- Public symbols must use the appropriate `DECL*` macros.
- Internal names start with a lower-case domain prefix; defines and enum values are all-uppercase.
- Functions normally return VBox status codes (`VERR_`, `VWRN_`, `VINF_`).
- Return `bool` only for true predicates and `void` only where failure is impossible.
- Keep the build warning-free across supported platforms.
- Do not use identifiers beginning with `_` or `__`.
- No `else` after an `if` block that ends in `return`, `break`, or `continue`.
- In Validation Kit Python code, use the documented variable prefixes:
  - type prefixes such as `f` for booleans, `i`/`l` for integers, `s` for strings, `o` for objects, `fn` for callables
  - collection qualifiers such as `a` for lists, `d` for dicts, `h` for sets, `t` for tuples
  - other qualifiers such as `c` for counts and `ms` / `us` / `ns` / `sec` for time-valued variables

Documentation rules from the same guide:

- New source files need the standard file header, including `$Id` and a short file description. This is generated by running `scm`.
- Public functions require Doxygen comments.
- Header-file structures must be documented, including members.
- Each module should have a Doxygen `@page` in its main source file.
- Non-obvious code must be explained with comments.
- Keep comments and documentation in sync with the code.
- In `include/VBox`, use ANSI C comments only, not `//`.

API compatibility rules:

- For `.xidl` / XPCOM / public API changes, call out the API impact explicitly.
- Minor-release API changes must follow the reserved-field and UUID rules.
- Major-release API changes still require correct UUID handling and sensible member placement.

Tree-wide style checks may involve `scm` when available. For newly added SVN
files, `svn-ps.sh` / `svn-ps.cmd` is the expected helper for setting properties.

## 7. Makefile Rules

The authoritative makefile guide is `doc/VBox-MakefileGuidelines.cpp`. These
rules are not optional for `Makefile.kmk` files.

Important rules:

- Everything is effectively global; avoid depending on parent, sibling, or uncle makefiles.
- Prefix makefile variables with `VBOX` or `VB`.
- Makefile variables are uppercase with underscores.
- Template names start with `VBox` and use camel case.
- Always use templates for targets.
- Put preprocessor defines in `DEFS`, not `FLAGS`.
- Keep custom recipes next to the targets they apply to where possible.
- Do not use custom recipes to install files; use install targets.
- Preserve deliberate blank-line structure.
- Use tabs only where make syntax requires them.

## 8. Subtree-Specific Guidelines

The tree-wide coding guide names local overrides that agents should consult when
working in these areas:

- `src/VBox/VMM/Docs-CodingGuidelines.cpp`
  - Distinguish guest-context and host-context pointers and addresses rigorously.
  - Use `RCPTRTYPE()`, `R0PTRTYPE()`, `R3PTRTYPE()`, `RTGCPTR`, `RTHCPTR`, `RTGCPHYS`, and `RTHCPHYS` correctly.
  - Never directly cast foreign-context pointers to local-context pointer types.

- `src/VBox/ValidationKit/ValidationKitCodingGuidelines.cpp`
  - Python variables use type/collection prefixes.
  - Python statements are conventionally terminated with semicolons.

- `src/VBox/Runtime/`
  - The optional VBox coding-guideline sections are treated as mandatory.

- `src/VBox/Main/`
  - Follow the `cppmain` section in `doc/VBox-CodingGuidelines.cpp`.

- `src/VBox/Frontends/VirtualBox/`
  - Follow the Qt GUI section in `doc/VBox-CodingGuidelines.cpp`.

For `src/libs/` and other imported third-party trees:

- Prefer no changes unless necessary.
- Preserve upstream/local style instead of reformatting to core VBox style.
- Keep VirtualBox-specific changes narrow and easy to rebase or diff.
- If the subtree already uses `#ifdef VBOX` integration points, follow the existing pattern rather than inventing a new one.

## 9. Security Rules

Follow `SECURITY.md` and the security-related documentation under `doc/manual/`.
Treat security review as a first-class requirement, not a postscript.

Mandatory security posture:

- Assume all guest-controlled, network-controlled, disk-controlled, and user-supplied input is hostile.
- This includes device emulation input, storage media contents, HGCM payloads, shared folders, drag and drop, clipboard traffic, guest-control traffic, XML/OVF metadata, and network packets.
- External strings are not trustworthy UTF-8; validate before interpreting or normalizing them.
- Guest code must not be able to compromise the host through memory corruption, unchecked parsing, privilege-boundary violations, or unexpected side effects.
- One guest must not be able to observe, influence, or tamper with another guest's private state or host-mediated communications.
- Guest Additions and host services are security-sensitive code, not convenience-only code.
- USB passthrough is high risk: a passed-through device is fully accessible to the guest. Changes in USB attachment/filtering paths need careful review.
- Prefer fail-closed behavior, explicit validation, narrow privilege, and authenticated/unguessable cross-boundary identifiers.

When a task touches a vulnerability or possible vulnerability:

- do not file or suggest a public GitHub issue for it
- do not add exploit code or unnecessary exploit detail to the repository
- minimize disclosure in code comments, commit text, and reviews
- follow `SECURITY.md`

Public reporting guidance in this repo is:

- `SECURITY.md`
- Oracle security reports go to `secalert_us@oracle.com`
- external reporters should use Oracle's vulnerability reporting process, not GitHub issues

Security-related feature ideas that are not vulnerability reports may still go
through normal GitHub issue flow.

## 10. Logging, Diagnostics, And Debugging

VirtualBox has extensive logging. Ask for or collect the minimum logs needed to
diagnose the target subsystem.

Common log artifacts seen in this tree and bugreports include:

- `VBox.log`
- `VBoxSVC.log`
- `VirtualBoxVM*.log`
- `VBoxNet*.log`

When debugging:

- request the smallest relevant log set
- avoid broad logging changes when narrow subsystem logging will do
- avoid collecting secrets, credentials, or unnecessary guest/user data
- document any extra logging knobs or reproduction steps you rely on

## 11. Documentation Expectations

If behavior changes, update the matching documentation:

- `doc/manual/` for user-visible behavior and command-line docs
- `doc/manual/user_ChangeLogImpl.xml` for user-visible fixes or changes
- `VBoxManage` manpage sources when flags or semantics change
- inline Doxygen / source comments when implementation details change

Do not leave user-visible behavior changed without updating the corresponding
manual or command documentation.

## 12. Contribution Workflow Notes

For GitHub-facing contributions, `CONTRIBUTING.md` adds requirements:

- non-security bugs and enhancements should have a GitHub issue
- pull requests require a signed Oracle Contributor Agreement
- commits intended for PRs need `Signed-off-by:`

Agents should not create commits on their own, but should keep these rules in
mind when preparing patches for external contribution.

If adding new SVN files for later human submission:

- preserve the standard file header format
- remind the human to use `svn-ps.sh -a` / `svn-ps.cmd -a` to set properties correctly

## 13. Review And Change Expectations

When proposing or completing a change, include:

1. the problem being fixed
2. the root cause
3. the minimal fix
4. tests run or a manual validation plan
5. compatibility, performance, and security risk notes
6. any API / `.xidl` impact

## 14. Quick Checklist

- [ ] Inspected local docs and the target subsystem before editing
- [ ] Kept the change minimal and localized
- [ ] Avoided unrelated refactors
- [ ] Followed `doc/VBox-CodingGuidelines.cpp`
- [ ] Followed `doc/VBox-MakefileGuidelines.cpp` for `Makefile.kmk` changes
- [ ] Checked subtree-specific guidelines where applicable
- [ ] Considered host/guest and guest/guest security boundaries
- [ ] Checked for possible memory leaks
- [ ] Avoided unnecessary churn in vendored code, toolchains, and generated files
- [ ] Built and tested the narrowest relevant targets when practical
- [ ] Documented untested areas and manual validation steps
- [ ] Updated manual/manpage/changelog docs for user-visible behavior changes
- [ ] Ran `scm` to check files against basic linter
- [ ] Did not create commits, branches, or pushes unless explicitly requested
