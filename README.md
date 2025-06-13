# Garlic decompiler
[![License](http://img.shields.io/:license-apache-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0.html)

Java/Android decompiler written in C

Tool for produces java source code from class/jar/dex/apk file

### Speed

decompile newest wechat.apk which size is 200M+ and 19w+ classes need 12 seconds

```sh
time -v ./cmake-build-debug/garlic ~/wechat/weixin8058android2841_0x28003a3f_arm64.apk
[Garlic] APK file analysis
File     : ~/wechat/weixin8058android2841_0x28003a3f_arm64.apk
Save to  : ~/wechat/weixin8058android2841_0x28003a3f_arm64_apk
Thread   : 4
Progress : 192538 (192538)
[Done]
	Command being timed: "./cmake-build-debug/garlic ~/wechat/weixin8058android2841_0x28003a3f_arm64.apk"
	User time (seconds): 32.57
	System time (seconds): 11.70
	Percent of CPU this job got: 351%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:12.58
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 584576
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 20
	Minor (reclaiming a frame) page faults: 45085
	Voluntary context switches: 16102
	Involuntary context switches: 310849
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 16384
	Exit status: 0

```

```sh
~/workspace/garlic $ hyperfine "./cmake-build-debug/garlic ~/wechat/weixin8058android2841_0x28003a3f_arm64.apk"
Benchmark 1: ./cmake-build-debug/garlic ~/wechat/weixin8058android2841_0x28003a3f_arm64.apk
  Time (mean ± σ):     11.502 s ±  0.105 s    [User: 31.803 s, System: 10.588 s]
  Range (min … max):   11.373 s … 11.774 s    10 runs
```

### Features

* decompile .class file
* decompile jar file
* decompile war file
* decompile .dex file
* decompile apk file

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

--------------------------------------------
*Licensed under the Apache 2.0 License*
