set UDK=edk2
set PROJECT_PKG=ValiantAppPkg
set PACKAGES_PATH=%CD%\%UDK%;%CD%\%PROJECT_PKG%

call %CD%\%UDK%\edk2setup.bat
build -p %WORKSPACE%\%PROJECT_PKG%\%PROJECT_PKG%.dsc --conf=%WORKSPACE%\%PROJECT_PKG%\Conf

PAUSE