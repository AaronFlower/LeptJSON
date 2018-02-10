# leptJSON

Guided by [miloyip](https://github.com/miloyip/json-tutorial). 

在 tutorial03 阶段的时候，我们可能需要测试内存是否泄漏。

在 XOS 系统中可以安装 [Vagrind](http://www.valgrind.org/) 来测试，[但是高版本的 OS 系统在安装 Vagrind 的时候可能不会成功](https://stackoverflow.com/questions/40650338/valgrind-on-macos-sierra/40713782#40713782)。

我们可以[利用 Xcode 的 instruments 来检测内存泄漏问题](https://help.apple.com/xcode/mac/9.0/index.html?localePath=en.lproj#/devcef23c572)。所以可以用 `cmake -G Xcode </path/to/CMakeLists.txt>` 来生成 Xcode 项目，但根据 tutorial 的 `CMakeLists.txt` 并不会引入 header 头文件到项目。为了使生成的项目引入所有的 `*.h, *.c` 文件，最终调整了下项目目录和 `CMakeLists.txt` 文件。

## 项目结构

```bash
> tree
.
├── CMakeLists.txt
├── changelog.md
├── readme.md
└── src
    ├── CMakeLists.txt
    ├── leptjson.c
    ├── leptjson.h
    └── test.c
```

## 命令行编译测试

```bash
# 当前项目目录
> mkdir build    
> cd build  
> cmake ..       
> make           
Scanning dependencies of target leptjson
[ 20%] Building C object src/CMakeFiles/leptjson.dir/leptjson.c.o
[ 40%] Linking C static library libleptjson.a
[ 40%] Built target leptjson
Scanning dependencies of target leptjson_test
[ 60%] Building C object src/CMakeFiles/leptjson_test.dir/leptjson.c.o
[ 80%] Building C object src/CMakeFiles/leptjson_test.dir/test.c.o
[100%] Linking C executable leptjson_test
[100%] Built target leptjson_test

> ./src/leptjson_
139/139 (100.00%) passed
```

## 生成 Xcode 项目

```bash
# 当前项目目录
> mkdir xcode
> cd xcode
> cmake -G Xcode ..  
> tree -I 1
.
├── CMakeCache.txt
├── CMakeFiles
├── CMakeScripts
├── cmake_install.cmake
├── leptJson.xcodeproj
└── src

4 directories, 2 files
```

用 Xcode 打开 `leptJson.xcodeproj` 即可。

## 参考

1. [Cmake and Xcode](https://kaushikghose.wordpress.com/2017/03/05/cmake-and-xcode/)
2. [Cmake by example](https://mirkokiefer.com/cmake-by-example-f95eb47d45b1)
3. [CMake Tutorial – Chapter 2: IDE Integration](https://www.johnlamp.net/cmake-tutorial-2-ide-integration.html)
4. [CMake & Xcode Tutorial (new)](https://www.youtube.com/watch?v=AmM0VEgfYWo)
