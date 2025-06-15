# Garlic decompiler
[![License](http://img.shields.io/:license-apache-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0.html)

中文 | [English](https://github.com/neocanable/garlic/blob/main/README.md)


用C语言实现的Android/Java反编译器


### 功能

* 反编译apk文件
* 反编译dex文件
* 反编译class文件
* 反编译jar文件
* 反编译war文件

### 编译

**编译环境**:  cmake版本: >=**3.26**

**没有其它依赖**

```sh
git clone https://github.com/neocanable/garlic.git
cd garlic
cmake -B build
cmake --build build
./build/garlic
```

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

```sh
$ hyperfine "garlic ~/wechat/wechat.apk"
Benchmark 1: garlic ~/wechat/wechat.apk
  Time (mean ± σ):     11.502 s ±  0.105 s    [User: 31.803 s, System: 10.588 s]
  Range (min … max):   11.373 s … 11.774 s    10 runs
```

### 定制服务

**邮件:** neocanable#gmail.com (替换 # 为 @)

**微信:** neocanable


![author](https://github.com/neocanable/garlic/blob/main/shell/images/qrcode.jpg)



--------------------------------------------
*Licensed under the Apache 2.0 License*
