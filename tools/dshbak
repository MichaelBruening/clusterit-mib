#!/bin/nawk -f 
# $Id: dshbak.src,v 1.5 2004/10/04 18:29:13 garbled Exp $
# dshbak  *must have nawk or compatible*
BEGIN { FS=":" }
{
	if ($1 != LASTNODE) {
		LASTNODE = $1
		printf("[Server: %s]\n", $1)
	}
	$1 = ""
	print substr($0, 3)
}
	
