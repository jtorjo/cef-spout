copy complete-git.py %1\automate /Y
c:
cd %1

rem set GN_DEFINES=is_official_build=true use_thin_lto=false proprietary_codecs=true ffmpeg_branding=Chrome media_use_ffmpeg=true media_use_libvpx=true  rtc_use_h264=true 
set GN_DEFINES=is_official_build=true use_thin_lto=false 

set GN_ARGUMENTS=--ide=vs2019 --sln=cef --filters=//cef/*
python .\automate\complete-git.py --download-dir=%1\chromium_git --force-clean --x64-build --url=https://bitbucket.org/rrjksn/cefsharedtextures --branch=4183
copy "%CEF_ROOT%_debug_symbols\libcef.dll.pdb" "%CEF_ROOT%\Debug" /Y
