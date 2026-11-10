# 4d-plugin-io
Replica of deprecated "Pack" commands 

## Native ownership maintenance

The bounded audit baseline is `edb67437c40c510450893291dbcf606846c091f1`.
Both Visual Studio projects and the Xcode sources phase compile `4DPlugin.cpp`,
the complete `4DPluginAPI.c`, and all 16 wrapper implementations. Their headers
are transitively included by `4DPluginAPI.h`; `C_TYPES.h` supplies string types
and platform imports. The native inventory comprises:

| Surface | Ownership finding |
|---|---|
| `4DPlugin.cpp`, `4DPlugin.h`, prefix header | `FourDPackex` calls `PluginMain`; selectors 1-6 implement fopen/fclose/fwrite/fread/fgetpos/fsetpos, matching both tracked manifests. No exported text arrays or picture commands. Scalar text inputs are copied by `C_TEXT`. Lifecycle helpers are not dispatched by the existing switch. |
| `C_TEXT.cpp/.h`, `ARRAY_TEXT.cpp/.h` | Borrowed parameter/array strings are copied into C++ strings. Scalar output uses the engine content setter; array output independently creates each string and transfers accepted assignments without local disposal. Normal replacement/shrink/clear are covered; failure boundaries below are not certified. |
| `C_PICTURE.cpp/.h` | Compiled but dormant from the current dispatcher. Fixed path alias double disposal, overwritten initialized string/picture owners, missing command-output clear after retain, and unreleased setBytes temporary. UTF-16 conversion bytes are divided by sizeof(PA_Unichar). Synchronous borrowed input detachment and owning picture returns are preserved. |
| `C_BLOB.cpp/.h` | Copies borrowed bytes into a vector and hands new handles to parameters/results. Text conversions use copied `C_TEXT` content; unrelated BLOB allocation-failure behavior is unchanged. |
| `C_POINTER.cpp/.h` | Compiled metadata wrapper; `PA_GetPointerValue` metadata/ownership interpretation is deferred, not blindly cleared. |
| `C_DATE`, `C_INTEGER`, `C_LONGINT`, `C_REAL`, `C_TIME` (`.cpp/.h`) | Scalar-only storage/access; no native string ownership. |
| `ARRAY_BOOLEAN`, `ARRAY_DATE`, `ARRAY_INTEGER`, `ARRAY_LONGINT`, `ARRAY_REAL`, `ARRAY_TIME` (`.cpp/.h`) | Numeric/date storage and array publication; no string owner cleanup. Initial creation/resize failure behavior is unchanged. |
| `4DPluginAPI.h/.c/.def`, `EntryPoints.h`, `Flags.h`, `PrivateTypes.h`, `PublicTypes.h`, `C_TYPES.h` | Bundled ABI, engine forwarding, exports and platform types inspected, unchanged. No object/collection/JSON first-party callback surface in this bundle. |

### Bundled contracts and provenance

The compiled SDK implementation has SHA-256
`2721c3d4338cc2a350599ffcee2a8008abaa756acfd45dd710720c64ca8c8ce3`,
identical to repository introduction commit
`2a3aae4f7d1fcb3cfe69c9c6203cffe0abbb4087`. Its implementation says v14 and
headers say v13; neither identifies a verified upstream SDK release.

`PA_SetStringInArray` accepts only ArrayUnicode with non-null data,
index >= 0 and <= element count, and non-null descriptor pointer. It disposes
the old element then shallow-assigns the incoming descriptor. Rejection retains
the caller owner; index zero is accepted. `PA_GetStringInArray` borrows.
`PA_SetStringVariable` and `PA_SetPictureVariable` shallow-assign without releasing
old contents; Undefined destinations avoid overwriting initial allocated owners.
`PA_SetUnistring`/`PA_ReturnString` instead forward to the content-setting engine
operation. Picture duplication with retainOnly=1 retains the input.

The picture callbacks are synchronous procedure commands. Their return is
normally Undefined; command argument-handle allocation failure returns raw
kind zero (misleadingly normalized to Unistring by `PA_GetVariableKind`).
That sentinel must not be cleared as an owned string. Method allocation
failure instead returns Undefined. The regression checks both actual SDK paths.

### Isolated regression and limits

On macOS with Apple clang and the platform frameworks, run without native
project build phases:

```sh
python3 -B tests/ownership/preserve.py
python3 -B tests/ownership/run.py --output /absolute/temporary/output
```

The harness compiles actual production sources, all wrappers and the FULL
UNCHANGED SDK. Only engine callback operations and dispatcher libc I/O are
stubbed; no real file, device or network input is opened. Exact ASCII, BMP,
supplementary and empty content, allocated/null empties, capacity/termination,
bytes-to-units conversion, array independence/replacement/shrink/clear/rejections,
borrowed copies, callback input/output lifetimes, failed poisoned picture-data
outputs and owning handoff survival are checked with ASan/UBSan and explicit
string/handle/picture balances. LeakSanitizer is unavailable on macOS; allocation
balance checks are not a claim of whole-process leak coverage.

The same harness accepts `--source` pointing to an immutable `git archive`
extraction of the baseline and `--expect-baseline-fail` to require the exact
named defect failures. The unsigned Ownership tests workflow also runs the
baseline differential and existing lifecycle tests; it does not sign, package,
upload artifacts, publish binaries, dispatch releases or alter manual workflows.

Deferred: ambiguous `PA_GetPictureData` string and SDK-only app-path ownership,
pointer metadata, initial array creation failure (the existing void wrapper
has no reliable error/preservation boundary), and SDK resize-OOM count
corruption. Normal-array tests do not certify OOM handling. No silent fallback,
SDK rewrite or blanket disposal removal was introduced. Existing unrelated IO
placeholder path/mode behavior and broad dispatcher exception swallowing are
unchanged. This is macOS synthetic-engine evidence, not execution in real 4D
or Windows and not certification of the original Poppler crash.

## Manual versioning and publication

Run **Bump version** manually with `patch`, `minor`, or `major`, selecting an
existing branch in the Actions UI. It checks out the immutable dispatch SHA,
validates a clean checkout and the remote branch, and commits/pushes only source
version metadata to that explicitly selected branch. An executable pre-push
guard validates the receive-pack advertisement: exactly one update, whose old
SHA is the dispatch SHA and whose literal new SHA is HEAD and its direct child.
Late branch advances, ancestor rewinds, deletions and same-result races fail
closed. Missing/non-executable guards also fail closed. Push is non-force with
both `push.followTags=false` and `--no-follow-tags`. Bump creates no tag and
does not trigger Build.

Run **Build** separately on the intended source branch. This workflow has only
`workflow_dispatch`, with **zero inputs**; all three checkouts use `github.sha`,
not a moving branch or tag. The strict source `X.Y.Z` determines `vX.Y.Z`,
`IO-X.Y.Z.zip` and `IO-X.Y.Z.dmg`. Existing Windows x64 and universal macOS
jobs, 4D resources, binary verification, signing, ZIP/DMG notarization and DMG
stapling remain mandatory before publication. There is no new default-branch
restriction. Publication uses one repository-wide concurrency group across
branches and never cancels a running publication.

### Source authority

`IO.xcodeproj/project.pbxproj`'s six `MARKETING_VERSION` settings are the source
authority. The existing Bump header introduced by `78f3a09` explicitly promised
coordinated `MARKETING_VERSION`/`CURRENT_PROJECT_VERSION` and plist version
fields. The source plist and all marketing settings reached **1.0.4** at
`ece7a1b`, while six current settings were left at `1.0.0` and the built resource
`English.lproj/InfoPlist.strings` still displayed `1.0`. This migration repairs
only those source fields to the existing **1.0.4**, without a version increment.
Future bumps validate and coordinate all six settings per key, existing
literal plist version fields and both localized display strings. Valid version
build-variable references remain references; absent `CFBundleVersion` stays
absent. BOMs, line endings, file modes, copyright text, prebuilt bundle metadata,
`locversion.plist`, manifests, 4D/schema counters and native dependencies are
not version inputs.

### Create-only publication and recovery

Before native work and again after **all** required build/sign/notary/staple
gates, publication checks the remote tag and every page of authenticated
releases, **including drafts**. Only a complete, valid no-match result permits
publication. Authentication, 404, network, malformed entries and later-page
failures stop the run; a by-tag release lookup is not sufficient.

Publication creates the lightweight tag with `POST git/refs` at the exact
selected SHA and verifies the remote SHA. Existing tags, including a concurrent
same-SHA creation, are errors. It then uses a separate create-only release POST
with explicit tag, selected `target_commitish`, matching name, `draft=false`,
`prerelease=false` and an empty body (the existing notes policy). Assets are
uploaded separately to the **newly returned release ID**. No update, delete,
clobber, rollback or retry is performed.

Failures retain any created tag, release and earlier uploaded assets. Inspect
the failed run, remote tag SHA, authenticated release list (including drafts),
release ID and assets before manual recovery. Do not rerun Build expecting it
to repair partial publication: existing state is deliberately rejected.
Maintainers must decide how to recover the retained state manually or choose
a subsequent source version. No workflow automatically removes failed state.

### Existing native caveats and local checks

The native projects retain the bundled 4D Plugin API, legacy Carbon prefix
header and deployment settings, Xcode Default configuration, checked-in
prebuilt bundle and Windows v140 project. Build retains its existing hosted
v143/Windows SDK 10.0 overrides and deployment-target override. Compatibility
of these legacy sources and checked-in outputs with current hosted toolchains
is not established by Python/workflow checks; native compilation, certificate
import, signing, notarization and stapling still have to succeed. The unchanged
secret names are `APPLE_DEVELOPER_ID_CERTIFICATE`,
`APPLE_DEVELOPER_ID_CERTIFICATE_PASSWORD`, `KEYCHAIN_PASSWORD`,
`NOTARYTOOL_APPLE_ID`, `NOTARYTOOL_TEAM_ID` and `NOTARYTOOL_PASSWORD`.
No native or signing gate is bypassed.

Local, non-native checks:

```sh
actionlint .github/workflows/bump-version.yml .github/workflows/release.yml
python3 -B -m unittest discover -s .github/tests -p 'test_*.py'
```

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|

### Version

<img src="https://cloud.githubusercontent.com/assets/1725068/18940649/21945000-8645-11e6-86ed-4a0f800e5a73.png" width="32" height="32" /> <img src="https://cloud.githubusercontent.com/assets/1725068/18940648/2192ddba-8645-11e6-864d-6d5692d55717.png" width="32" height="32" />

[4D_Pack : ANSI Streams](http://doc.4d.com/4Dv15/4D-Pack/15.1/4D-Pack-ANSI-Streams.201-2787480.en.html)

* _o_AP FCLOSE
* _o_AP fopen
* _o_AP FPRINT
* _o_AP fread

On Windows, [_wfopen](https://msdn.microsoft.com/en-us/library/yeby3zcb.aspx) is used internally to work with Unicode paths.

On macOS, the path is internally converted from HFS to POSIX.

Unlike the original functions, the plugin works with BLOBs, not ANSI text.

## Example 

```
$path:=System folder(Desktop)+"test.txt"

$fp:=fopen ($path;"w+")

CONVERT FROM TEXT("abcdefghijklmnop";"utf-8";$data)
$written:=fwrite ($fp;$data)

$err:=fgetpos ($fp;$pos)

$err:=fsetpos ($fp;5)
CLEAR VARIABLE($data)
$read:=fread ($fp;$data)  //read to end of file (11 bytes)
$text:=Convert to text($data;"utf-8")  //fghijklmnop

$err:=fsetpos ($fp;0)
CLEAR VARIABLE($data)
$read:=fread ($fp;$data;5)  //read 5 bytes
$text:=Convert to text($data;"utf-8")  //abcde

fclose ($fp)
```

## Syntax

```
file:=fopen (path;mode)
```

Parameter|Type|Description
------------|------------|----
path|TEXT|
mode|TEXT|
file|LONGINT|

```
fclose  (file)
```

Parameter|Type|Description
------------|------------|----
file|LONGINT|

```
error:=fsetpos (file;pos)
error:=fsetpos (file;pos)
```

Parameter|Type|Description
------------|------------|----
file|LONGINT|
pos|LONGINT|
error|LONGINT|

```
read:=fread (file;data{;length})
```

Parameter|Type|Description
------------|------------|----
file|LONGINT|
data|BLOB|
length|LONGINT|
read|LONGINT|


```
written:=fwrite (file;data)
```

Parameter|Type|Description
------------|------------|----
file|LONGINT|
data|BLOB|
written|LONGINT|
