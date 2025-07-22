## build garlic on windows





#### Setup MinGW

use [winlibs](https://winlibs.com/), download url is: [mingw](https://github.com/brechtsanders/winlibs_mingw/releases/download/15.1.0posix-12.0.0-msvcrt-r1/winlibs-x86_64-posix-seh-gcc-15.1.0-mingw-w64msvcrt-12.0.0-r1.zip)

unzip it to ```D:\winlibs```

Add ```d:\winlibs\mingw64\bin``` to PATH variables



### Setup cmake

use [cmake installer](https://github.com/Kitware/CMake/releases/download/v4.1.0-rc2/cmake-4.1.0-rc2-windows-x86_64.msi)

just click install

**Make sure the environment variable PATH contains the cmake bin directory**



#### Build

###### 1. config

**git bash/powershell/windows terminal**

``` shell
 cmake -B ./build/ -G "MinGW Makefiles"
```

**cmd.exe**

```shell
cmake -B build -G "MinGW Makefiles"
```



###### 2. build

```shell
cmake --build build
```





