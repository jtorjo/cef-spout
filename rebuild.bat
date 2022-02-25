rmdir build /Q /S
dir build
pause
mkdir build
cd build
cmake -D USE_SANDBOX=OFF ..
pause
