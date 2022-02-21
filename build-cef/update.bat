rem get automate-git.py from https://bitbucket.org/chromiumembedded/cef/raw/master/tools/automate/automate-git.py

c:
cd C:\code

rem set GN_DEFINES=is_official_build=true use_thin_lto=false proprietary_codecs=true ffmpeg_branding=Chrome media_use_ffmpeg=true media_use_libvpx=true  rtc_use_h264=true 
set GN_DEFINES=is_official_build=true use_thin_lto=false 

set GN_ARGUMENTS=--ide=vs2019 --sln=cef --filters=//cef/*
rem python ..\automate\automate-git.py --download-dir=c:\code\chromium_git --branch=4844 --minimal-distrib --client-distrib --force-clean --x64-build
python automate\automate-git.py --download-dir=c:\code\chromium_git --branch=4844 --force-clean --x64-build
