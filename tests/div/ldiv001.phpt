--TEST--
ldiv
--INI--
psi.directory={PWD}:{PWD}/../../psi.d
--FILE--
===TEST===
<?php
var_dump(psi\ldiv(1000,10));
?>
===DONE===
--EXPECT--
===TEST===
array(2) {
  ["quot"]=>
  int(100)
  ["rem"]=>
  int(0)
}
===DONE===
