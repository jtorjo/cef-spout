rmdir build /Q /S
pause
mkdir build
cd build
cmake -D USE_SANDBOX=OFF ..
pause
