# garlic-decompiler
[![License](http://img.shields.io/:license-apache-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0.html)

Java decompiler written in C

Tool for produces java source code from .class or jar file

### Features

* decompile .class file
* decompile jar file
* decompile war file

### Build

**requirements**: cmake >= **3.26**

**No other dependencies**

```sh
git clone https://github.com/neocanable/garlic.git
cd garlic
cmake -B build
cmake --build build
./build/garlic
```

### Usage

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
    garlic /path/to/dalvik.dex -p           # unsupport now

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

--------------------------------------------
*Licensed under the Apache 2.0 License*
