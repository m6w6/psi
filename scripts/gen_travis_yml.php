#!/usr/bin/env php
# autogenerated file; do not edit
language: c
sudo: required
dist: trusty

addons:
 apt:
  packages:
   - php5-cli
   - php-pear
   - valgrind
   - re2c
   - libidn11-dev
   - libsqlite3-dev

env:
 matrix:
<?php

$gen = include __DIR__."/../travis/pecl/gen-matrix.php";
$env = $gen([
	"PHP" => ["master"],
	"enable_debug",
	#"enable_maintainer_zts",
	"enable_psi" => ["yes"],
	"enable_psi_posix" => ["all"],
]);
foreach ($env as $e) {
	printf("  - %s\n", $e);
}

?>

before_script:
 # make sure we do not try to regenerate files with broken bison
 - touch src/parser*.[ch]
 - make -f travis/pecl/Makefile php
 - make -f travis/pecl/Makefile ext PECL=psi

script:
 - make -f travis/pecl/Makefile test

after_failure:
 - cat config.log | curl -F 'sprunge=<-' http://sprunge.us
 - cat tests/parser/dump001.psi
 - ldd .libs/psi.so

notifications:
  webhooks:
    urls:
      - https://webhooks.gitter.im/e/30aebb6c0b03f1117817
    on_success: change
    on_failure: always
    on_start: never
