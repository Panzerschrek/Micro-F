rm Micro-F-upx.exe
/C/lnk/upx391w/upx.exe Micro-F.exe -o Micro-F-upx.exe -9 --brute --ultra-brute --best --no-align
du Micro-F-upx.exe -b | cut -f 1 >> size_log.txt
date +%H:%M\ %e.%m.%Y >> size_log.txt
echo \
 >> size_log.txt
