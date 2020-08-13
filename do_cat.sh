#! /bin/sh
echo 'building dominion.tex from dom_tex.*'
cat dom_tex.aa dom_tex.ab > dominion.tex
echo 'building dominion.info from dom_info.*'
cat dom_info.aa dom_info.ab | uudecode
echo 'building dominion.PS from dom_ps.*'
cat dom_ps.a[a-j] | uudecode
rm dom_tex.* dom_info.* dom_ps.*
