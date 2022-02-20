copy complete-git.py c:\code\automate /Y
c:
cd c:\code\chromium_git

set GN_DEFINES=is_official_build=true use_thin_lto=false proprietary_codecs=true ffmpeg_branding=Chrome media_use_ffmpeg=true media_use_libvpx=true  rtc_use_h264=true 

set GN_ARGUMENTS=--ide=vs2019 --sln=cef --filters=//cef/*
rem python ..\automate\complete-git.py --download-dir=c:\code\chromium_git --branch=4844 --minimal-distrib --client-distrib --force-clean --x64-build
python ..\automate\complete-git.py --download-dir=c:\code\chromium_git --branch=4844 --force-clean --x64-build
copy "%CEF_ROOT%_debug_symbols\libcef.dll.pdb" "%CEF_ROOT%\Debug" /Y
