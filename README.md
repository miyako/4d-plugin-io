# 4d-plugin-io
Replica of deprecated "Pack" commands 

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
