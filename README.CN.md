# Garlic decompiler
[![License](http://img.shields.io/:license-apache-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0.html)

[![Telegram](https://img.shields.io/badge/Telegram-Join%20Chat-blue?logo=telegram)](https://t.me/+8N--zZal0KExMzA1)


中文 | [English](README.md)

世界最快的 APK (Android)/Java 开源反编译器

用 C 语言实现的 Android/Java 反编译器

从 class/jar/dex/apk 文件生成 Java 源码的工具


### 功能

* 反编译apk文件
* 反编译dex文件
* 反编译class文件
* 反编译jar文件
* 反编译war文件

### 安装

* macos/linux
``` shell
brew install neocanable/decompiler/garlic
```

### 编译

##### 1. 在Linux/macOS上编译

​	**编译环境**:  cmake版本: >=**3.26**

​	**没有其它依赖**

```sh
git clone https://github.com/neocanable/garlic.git
cd garlic
cmake -B build
cmake --build build
./build/garlic
```

##### 2. 在windows上编译

​	请查看在[windows编译方法](docs/build-garlic-on-windows.md)

	另见 [Windows 使用指南](docs/garlic-on-windows.md) 了解性能优化和已知问题。

##### 3. 使用 Zig 编译 (跨平台)

​	**编译环境**: zig >= **0.16.0**

直接编译本机平台：

```sh
git clone https://github.com/neocanable/garlic.git
cd garlic
zig build --release=fast
./zig-out/bin/garlic
```

使用 `-Dtarget` 交叉编译到任意目标平台：

```sh
# Linux x86_64 (musl)
zig build --release=fast -Dtarget=x86_64-linux-musl

# Linux x86_64 (glibc)
zig build --release=fast -Dtarget=x86_64-linux-gnu

# Linux aarch64
zig build --release=fast -Dtarget=aarch64-linux-musl

# Linux i686 (32位)
zig build --release=fast -Dtarget=x86-linux-musl

# Windows x86_64
zig build --release=fast -Dtarget=x86_64-windows

# Windows 32位
zig build --release=fast -Dtarget=x86-windows

# macOS x86_64 (Intel)
zig build --release=fast -Dtarget=x86_64-macos

# macOS aarch64 (Apple Silicon)
zig build --release=fast -Dtarget=aarch64-macos
```

Zig 自带交叉链接器和 libc，**无需安装任何交叉编译工具链**，开箱即用。产物输出到 `zig-out/bin/`。



### 使用方法

* 反编译apk
  ```sh
  garlic /path/to/android.apk
  
  garlic /path/to/android.apk -o /path/to/save # -o 选项是源码输出的目录
  
  garlic /path/to/android.apk -t 5             # -t 选项是线程数量, 默认是4
  ```

* 反编译dex
  ```sh
  garlic /path/to/classes.dex
  
  garlic /path/to/classes.dex -o /path/to/save # -o 选项是源码输出的目录
  
  garlic /path/to/classes.dex -t 5             # -t 选项是线程数量, 默认是4
  ```

* 反编译 .class 文件

    反编译 .class 文件默认输出是**stdout**
    ```sh
    garlic /path/to/jvm.class
    ```


* 反编译 jar 文件
    ```sh
    garlic /path/to/file.jar
    
    garlic /path/to/file.jar -o /path/to/save # -o 选项是源码输出的目录
    
    garlic /path/to/file.jar -t 4             # -t 选项是线程数量, 默认是4
    ```

    没有制定输出目录情况下，默认输出目录是jar的同级目录。


* javap 
  
    像javap，比javap快一些，关闭了LineNumber和StackMapTable属性输出。
    ```sh
    garlic /path/to/jvm.class -p
    ```

* dexdump
    ```sh
    garlic /path/to/dalvik.dex -p 
    
    ```
* 搜索字符串
  ```
  garlic ~/demo/demo.apk -f "windowInfo" # search "windowInfo" in demo.apk
  ```

  ```
  garlic ~/demo/demo.jar -f "[W|w]indow" # search regex [W|w]indow in demo.jar
  ```

  ```
  garlic ~/demo/demo.dex -f "info" # search contains string in demo.dex
  ```

### 调试

修改 **src/jvm.c** 中的 main 函数:

```c
int main(int argc, char **argv)
{
    jar_file_analyse(path_of_jar, out_of_jar, 1);
    return 0;
}

```

如果线程数小于 2，则会禁用多线程。



### 速度

反编译最新（2025-06-16）的微信，200M+大小, 19万+个类，需要12秒左右，默认4线程

```sh
garlic ~/wechat/wechat.apk
[Garlic] APK file analysis
File     : ~/wechat/wechat.apk
Save to  : ~/wechat/wechat_apk
Thread   : 4
Progress : 192538 (192538)
[Done]
```


![decompile tiktok](shell/images/garlic-show.gif)


### Garlic + Rosemary 流水线

[视频](https://youtu.be/I_cwuW4UKOs?si=eTCFuC1XzHuBi5a0)


### 定制服务

**邮件:** neocanable#gmail.com (替换 # 为 @)

**微信:** neocanable


![author](shell/images/qrcode.jpg)



--------------------------------------------
*Licensed under the Apache 2.0 License*
