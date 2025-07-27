# Garlic decompiler
[![License](http://img.shields.io/:license-apache-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0.html)

English | [Chinese](https://github.com/neocanable/garlic/blob/main/README.CN.md)

The world's fastest apk (android)/java open source decompiler

Android/Java decompiler written in C

Tool for produces java source code from class/jar/dex/apk file

### Features

* decompile apk file
* decompile dex file
* decompile class file
* decompile jar file
* decompile war file

### Install

* macos/linux
``` shell
brew install neocanable/decompiler/garlic
```


### Build

##### 1. Build on linux/macOS 

​	**requirements**: cmake >= **3.26**


```sh
git clone https://github.com/neocanable/garlic.git
cd garlic
cmake -B build
cmake --build build
./build/garlic
```



##### 2. Build on Windows

please check the [windows build document](https://github.com/neocanable/garlic/blob/main/docs/build-garlic-on-windows.md)



### Usage

* decompile apk
  ```sh
  garlic /path/to/android.apk
  
  garlic /path/to/android.apk -o /path/to/save # -o option is source code output path
  
  garlic /path/to/android.apk -t 5             # -t option is thread count, default is 4
  ```

* decompile .dex file
  ```sh
  garlic /path/to/classes.dex
  
  garlic /path/to/classes.dex -o /path/to/save # -o option is source code output path
  
  garlic /path/to/classes.dex -t 5             # -t option is thread count, default is 4
  ```

* decompile .class file

    decompile .class file, default output is **stdout**
    ```sh
    garlic /path/to/jvm.class
    ```


* decompile jar file
    ```sh
    garlic /path/to/file.jar
    
    garlic /path/to/file.jar -o /path/to/save # -o option is source code output path
    
    garlic /path/to/file.jar -t 5             # -t option is thread count, default is 4
    ```

    default output is same level directory as the file


* javap 
  
    like javap, more faster, disabled LineNumber and StackMapTable attributes
    ```sh
    garlic /path/to/jvm.class -p
    ```

* dexdump
    ```sh
    garlic /path/to/dalvik.dex -p           
    
    ```


### Debug

in **src/jvm.c**, change main function to: 

```c
int main(int argc, char **argv)
{
    jar_file_analyse(path_of_jar, out_of_jar, 1);
    return 0;
}

```

if thread count less than 2, it will disable multiple thread.



### Speed

decompile newest(2025-06-16) wechat.apk which size is 200M+ and 19w+ classes need 12 seconds

```sh
garlic ~/wechat/wechat.apk
[Garlic] APK file analysis
File     : ~/wechat/wechat.apk
Save to  : ~/wechat/wechat_apk
Thread   : 4
Progress : 192538 (192538)
[Done]
```

![decompile tiktok](https://github.com/neocanable/garlic/blob/main/shell/images/garlic-show.gif)


### Customization

**Email:**   neocanable#gmail.com (replace # to @)

**wechat:** neocanable


![author](https://github.com/neocanable/garlic/blob/main/shell/images/qrcode.jpg)


--------------------------------------------
*Licensed under the Apache 2.0 License*
