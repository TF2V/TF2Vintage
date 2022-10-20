@echo off
cls
	cmake -S . -B build -A Win32 -DBUILD_GROUP=everything
@pause