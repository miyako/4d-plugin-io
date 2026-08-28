//%attributes = {}
$path:=System folder:C487(Desktop:K41:16)+"test.txt"

$fp:=fopen($path; "w+")

CONVERT FROM TEXT:C1011("abcdefghijklmnop"; "utf-8"; $data)
$written:=fwrite($fp; $data)

$err:=fgetpos($fp; $pos)

$err:=fsetpos($fp; 5)
CLEAR VARIABLE:C89($data)
$read:=fread($fp; $data)  //read to end of file (11 bytes)
$text:=Convert to text:C1012($data; "utf-8")  //fghijklmnop

$err:=fsetpos($fp; 0)
CLEAR VARIABLE:C89($data)
$read:=fread($fp; $data; 5)  //read 5 bytes
$text:=Convert to text:C1012($data; "utf-8")  //abcde

fclose($fp)