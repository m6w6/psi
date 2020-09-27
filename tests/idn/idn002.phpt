--TEST--
libidn
--INI--
psi.directory={PWD}/../../psi.d:{PWD}
--FILE--
===TEST===
<?php

$rc = idn\utf8_to_ascii("…asdf….de", $result, psi\IDNA_USE_STD3_ASCII_RULES);
printf("%s\n", $result);
printf("%s\n", idn\strerror($rc));

?>
===DONE===
--EXPECT--
===TEST===

Non-digit/letter/hyphen in input
===DONE===
