#!/bin/sh
files="mymodule.cpp"
main="main.cpp"
output="copy"
if [ -f "./gcm.cache/std.gcm" ]; then
	echo "[LOG] compiling the the the translation units only"
	g++ -std=c++23 -fmodules "$files" "$main" -o "$output"
else
	echo "[LOG] compiling the std and the translation units"
	g++ -std=c++23 -fmodules -fsearch-include-path bits/std.cc "$files" "$main" -o "$output"
fi
