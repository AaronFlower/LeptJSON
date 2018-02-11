# leptJSON

Guided by [miloyip](https://github.com/miloyip/json-tutorial). 

在 tutorial03 阶段的时候，我们可能需要测试内存是否泄漏。

在 XOS 系统中可以安装 [Vagrind](http://www.valgrind.org/) 来测试.

我们也可以[利用 Xcode 的 instruments 来检测内存泄漏问题](https://help.apple.com/xcode/mac/9.0/index.html?localePath=en.lproj#/devcef23c572)。所以可以用 `cmake -G Xcode </path/to/CMakeLists.txt>` 来生成 Xcode 项目，但根据 tutorial 的 `CMakeLists.txt` 并不会引入 header 头文件到项目。为了使生成的项目引入所有的 `*.h, *.c` 文件，最终调整了下项目目录和 `CMakeLists.txt` 文件。

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

# Valgrind 测试
> valgrind --leak-check=full ./src/leptjson_test  
```

### valgrind uninitialised value
```c
static void test_access_boolean () {
    lept_value v;
//  lept_init(&v);
  lept_set_string(&v, "a", 1);
    lept_set_boolean(&v, 1);
    EXPECT_EQ_BOOLEAN(LEPT_TRUE, lept_get_boolean(&v));
    lept_set_boolean(&v, 0);
    EXPECT_EQ_BOOLEAN(LEPT_FALSE, lept_get_boolean(&v));
    lept_free(&v);
}
```
注释 `//  lept_init(&v);` 将会导致 `v->type` 未初始化，面之后对来回改变 `v-type` 的值为 `LEPT_STRING`, `LEPT_TRUE`, `LEPT_FALSE`。 
用 valgrind 检测出是 ` Conditional jump or move depends on uninitialised value(s)`。 这样发布会出现程序崩溃的。

```
$ valgrind --leak-check=full ./src/leptjson_test                                                                                                 ‹ruby-2.2.4›
==92309== Memcheck, a memory error detector
==92309== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==92309== Using Valgrind-3.14.0.GIT and LibVEX; rerun with -h for copyright info
==92309== Command: ./src/leptjson_test
==92309==
--92309-- run: /usr/bin/dsymutil "./src/leptjson_test"
==92309== Conditional jump or move depends on uninitialised value(s)
==92309==    at 0x1000010C4: lept_free (leptjson.c:307)
==92309==    by 0x1000011FA: lept_set_string (leptjson.c:317)
==92309==    by 0x1000075BE: test_access_boolean (test.c:166)
==92309==    by 0x100001DD0: test_parse (test.c:205)
==92309==    by 0x100001D43: main (test.c:211)
```

### valgrind memory leak
如果要在设置非字符值时，而忘记释放之前分配的字符串内存，那么就会有内存泄漏。
```c
void lept_set_boolean(lept_value* v, int b) {
    // lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}
```

用 `valgrind` 检测 `2 bytes in 1 blocks are definitely lost in loss record 1 of 60`

```bash
==77823== HEAP SUMMARY:
==77823==     in use at exit: 24,767 bytes in 189 blocks
==77823==   total heap usage: 216 allocs, 27 frees, 33,487 bytes allocated
==77823==
==77823== 2 bytes in 1 blocks are definitely lost in loss record 1 of 60
==77823==    at 0x1000B4996: malloc (in /usr/local/Cellar/valgrind/HEAD-1cb4ab6/lib/valgrind/vgpreload_memcheck-amd64-darwin.so)
==77823==    by 0x100001087: lept_set_string (leptjson.c:337)
==77823==    by 0x1000075C5: test_access_boolean (test.c:166)
==77823==    by 0x100001DD0: test_parse (test.c:205)
==77823==    by 0x100001D43: main (test.c:211)
==77823==
==77823== 72 bytes in 3 blocks are possibly lost in loss record 37 of 60
==77823==    at 0x1000B5002: calloc (in /usr/local/Cellar/valgrind/HEAD-1cb4ab6/lib/valgrind/vgpreload_memcheck-amd64-darwin.so)
==77823==    by 0x1006C97E2: map_images_nolock (in /usr/lib/libobjc.A.dylib)
==77823==    by 0x1006DC7DA: objc_object::sidetable_retainCount() (in /usr/lib/libobjc.A.dylib)
==77823==    by 0x10000FC64: dyld::notifyBatchPartial(dyld_image_states, bool, char const* (*)(dyld_image_states, unsigned int, dyld_image_info const*), bool, bool) (in /usr/lib/dyld)
==77823==    by 0x10000FE39: dyld::registerObjCNotifiers(void (*)(unsigned int, char const* const*, mach_header const* const*), void (*)(char const*, mach_header const*), void (*)(char const*, mach_header const*)) (in /usr/lib/dyld)
==77823==    by 0x10022981D: _dyld_objc_notify_register (in /usr/lib/system/libdyld.dylib)
==77823==    by 0x1006C9075: _objc_init (in /usr/lib/libobjc.A.dylib)
==77823==    by 0x1001B3ACC: _os_object_init (in /usr/lib/system/libdispatch.dylib)
==77823==    by 0x1001B3AB3: libdispatch_init (in /usr/lib/system/libdispatch.dylib)
==77823==    by 0x1000C39C2: libSystem_initializer (in /usr/lib/libSystem.B.dylib)
==77823==    by 0x100021A09: ImageLoaderMachO::doModInitFunctions(ImageLoader::LinkContext const&) (in /usr/lib/dyld)
==77823==    by 0x100021C39: ImageLoaderMachO::doInitialization(ImageLoader::LinkContext const&) (in /usr/lib/dyld)
==77823==
==77823== LEAK SUMMARY:
==77823==    definitely lost: 2 bytes in 1 blocks
==77823==    indirectly lost: 0 bytes in 0 blocks
==77823==      possibly lost: 72 bytes in 3 blocks
==77823==    still reachable: 1,768 bytes in 28 blocks
==77823==         suppressed: 22,925 bytes in 157 blocks
==77823== Reachable blocks (those to which a pointer was found) are not shown.
==77823== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==77823==
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
