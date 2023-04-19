@echo off

cl -nologo -std:c++20 -W3 -Zi -EHsc lbfnt.cpp -I"../../src/" -Fe"../../build/lbfnt.exe" /link /SUBSYSTEM:CONSOLE