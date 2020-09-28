rem Copyright 2020 The Mumble Developers. All rights reserved.
rem Use of this source code is governed by a BSD-style license
rem that can be found in the LICENSE file at the root of the
rem source tree.

mkdir build
cd build
cmake -Dtests=ON -DVCPKG_TARGET_TRIPLET="x64-windows" -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" ..

if %errorlevel% NEQ 0 (
	exit /b %errorlevel%
)

SET LIB="%LIB%;C:\hostedtoolcache\windows\boost\1.72.0\x86_64\lib\"

cmake --build . --config Release
