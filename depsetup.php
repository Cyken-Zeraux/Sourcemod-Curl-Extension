#!/usr/bin/env php
<?php
//Currently Linux only, partially setup for multi-platform

$home = getenv('$HOME');
$sources_path = realpath($home . DIRECTORY_SEPARATOR . 'sources')

$dir = new DirectoryIterator($sources_path);
foreach ($dir as $fileinfo) {
    if (!$fileinfo->isDir()) {
    	$fname = $fileinfo->getBasename();
    	$fpathname = $fileinfo->getPathname();
    	$fpath = $fileinfo->getPath();
        $pos = stripos($fname, "openssl");
        if($pos === false) {
        	exit("could not find openssl");
        }
        $ren = rename($fpathname, $fpath . DIRECTORY_SEPARATOR . 'openssl');
    	if($ren !== true) {
    		exit("could not rename openssl source");
    	}
    }
}

?>