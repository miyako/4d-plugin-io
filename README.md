# 4d-plugin-io
Replica of deprecated "Pack" commands 

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
