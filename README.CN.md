# garlic-decompiler
[![License](http://img.shields.io/:license-apache-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0.html)

Java反编译器C语言实现


### 功能

* 反编译.class文件
* 反编译.jar文件
* 反编译.war文件

### 编译

**编译环境**:  cmake版本: >=**3.26**

**没有其它依赖**

```sh
git clone https://github.com/neocanable/decompiler-garlic.git
cd decompile-garlic
cmake -B build
cmake --build build
./build/garlic
```

### 使用方法

* 反编译 .class 文件

    反编译 .class 文件默认输出是**stdout**
    ```sh
    garlic /path/to/jvm.class
    ```


* 反编译 jar 文件
    ```sh
    garlic /path/to/file.jar

    garlic /path/to/file.jar -o /path/to/save # -o 选项是源吗输出的目录
    
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
    garlic /path/to/dalvik.dex -p           # unsupport now

    ```

--------------------------------------------
*Licensed under the Apache 2.0 License*