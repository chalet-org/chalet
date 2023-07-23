"%ProgramFiles%\Git\bin\bash.exe" ./fetch_externals.sh

.\build.bat Debug && build\msvc_Debug\chalet-debug -c Release buildrun

echo "exited with code %errorlevel%"
