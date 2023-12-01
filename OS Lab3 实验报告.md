# OS Lab3 实验报告

<div style="text-align:center">王梓萌 521030910015</div>



## 练习题0

按照题目中所给信息安装 github submodule 即可

> > `musl-libc`是一个C标准库的实现，它为Linux系统上的应用程序提供核心的系统函数和服务。由于其设计简洁、轻量以及许可证宽松（MIT许可证），`musl-libc`适用于静态链接，并且常被用于嵌入式系统、容器环境以及任何对资源使用敏感的场景。
> >
> > `musl-libc`的目的是提供一个简单、高效且标准遵从的C库，它的目标是完全兼容POSIX以及ISO C标准。`musl-libc`提供的功能包括字符串操作、文件和目录操作、网络通信、内存管理、程序执行控制等。
> >
> > 与其他C标准库实现如`glibc`（GNU C Library）相比，`musl-libc`关注于简洁和正确性，以及避免不必要的复杂性和bug。它容易被静态链接到应用程序中，这有助于减少应用程序的运行时依赖。静态链接也使得程序的分发和部署变得更加简单，因为不需要担心系统上的动态库版本问题。
> >
> > 由于上述特点，`musl-libc`在容器化应用程序、Linux发行版（如Alpine Linux）和其他需要精简系统环境的场合中非常流行。



## 练习题1

在 `create_root_cap_group` 函数中，我们需要完成第一个进程的创建

Chcore 使用 Capability 机制作为进程的实现，使得操作系统以进程为粒度进行权限管理。进程中的`cap_table` 中每个表项记录了一个进程是否可以访问到一个特定的 `kernel object` ， 对应的表项就是 `#cap_t` 类型的capability标号。因此操作系统可以轻易地回收或者赋予一个进程访问 `kernel object` 的权限。

为了完成root进程的创建，我们需要创建 `cap_group`本身并初始化，同时需要创建其`vmspace`同时初始化，最后需要将二者都记录在 `cap_table`之中。为了完成用户态进程的创建，我们额外可以看到在进程初始化的时候，加入了很多从syscall中传入的信息，比如进程的badge等等。

具体在代码中，我们首先需要使用 `obj_alloc` 函数分配内存空间创建所想要类型的内核对象，然后使用 `**_init` 等函数进行初始化操作，最后我们使用`cap_alloc` 函数将初始化好的`cap_group` 和 `vmspace` 放入 `cap_group` 的 `cap_table` 中，可以形象地认为这个进程具有权限访问这些内核对象。

具体代码见源代码



## 练习题2

线程是实际执行被操作系统调度和用来创建执行具体程序代码的对象。线程也是一个内核对象，且必须挂靠在一个进程之下，这在我们刚才的 `cap_group` 结构体中可以看到。

线程的初始化方法则更为地贴合执行具体程序的要求。

首先需要进行虚拟地址空间和代码环境加载root线程需要从镜像文件的对应elf位置加载程序的代码到分配好的虚拟内存地址，普通线程则由直接可以读取存储在内存中的elf文件得到程序的具体信息。同时需要初始化线程本身的vmspace，以及线程的上下文等信息。这些都需要被记录线程所属 `cap_group` 的 `cap_table`中。其余部分比如创建内核，用户栈等操作，代码已经在函数中给出。

从syscall中创建其他线程这部分不在本实验的范围之内。

具体代码见源代码



## 练习题3

完成虚拟地址空间和代码环境加载后，还需要初始化处理器上下文，或者说初始化通用寄存器的值和特殊寄存器的值，特殊寄存器中例如要填写SPSR，知道eret指令返回的特权级等等。

具体详见源代码



## 思考题4

使用gdb追踪发现，函数调用的主要路径为：

```

eret_to_thread() -> __eret_to_thread() -> set_exception_table() -> sync_el0_64() -> handle_entry_c() -> do_page_fault() -> ... -> handle_trans_fault() -> handle_entry_c() -> sync_el0_64() -> handle_entry_c() -> sync_el0_64() -> 进入用户态 ->el0_syscall() -> ...


```

因此可以认为，在进入用户态之前，程序首先从main函数通过switch_context 初始化线程上下文，然后调用eret_to_thread(）函数，填写了异常向量表。然后调用了同步异常的处理函数，通过syscall() 返回的方式，返回进入 el0, 执行我们之前配置的从镜像加载到线程地址空间的代码，直至程序能够正常运行.



## 练习题5

异常向量表由操作系统填写，实际是由硬件去使用。硬件通过特殊寄存器（比如当前特权等级，异常原因等），选择已经填写好的异常向量表中的合适表项，跳转到对应的处理函数，交由操作系统负责。异常向量表的填写方式和PPT课件上所展示的填写方法完全一样。

具体详见源代码



## 练习题6

`exception_enter`  和 `exception_exit` 是操作系统代码在进入异常处理函数时第一个和最后一个执行的代码块。其作用是保存所有用户状态（处理器上下文），然后再完成系统调用后，恢复处理器上下文，并将返回值存在x0寄存器中。

具体详见源代码



## 思考7

printf的调用方式如下所示

1. **`printf` 调用 `vprintf`**， **`vprintf` 调用 `vfprintf`**：

   `printf` 是一个可变参数函数，它内部调用 `vfprintf` 来处理参数列表和格式化输出。`vfprintf` 是处理变长参数列表的核心函数。

2. **`vfprintf` 处理 `stdout`**: 

   `vfprintf` 接收一个文件描述符作为输出目标。在 `printf` 的情况下，这个文件描述符是 `stdout`。在项目中的代码具体为

   ```c
   __fortify_function int
   vprintf (const char *__restrict __fmt, __gnuc_va_list __ap)
   {
   #ifdef __USE_EXTERN_INLINES
     return __vfprintf_chk (stdout, __USE_FORTIFY_LEVEL - 1, __fmt, __ap);
   #else
     return __vprintf_chk (__USE_FORTIFY_LEVEL - 1, __fmt, __ap);
   #endif
   }
   ```

3. **`stdout` 的写操作被定义为 (`__stdout_write`)** **其中又调用 `__stdio_write`**:

   ```c
   
   // stdout.c 文件
   static unsigned char buf[BUFSIZ+UNGET];
   hidden FILE __stdout_FILE = {
   	...
   	.write = __stdout_write,
   
   };
   
   //__stdout_write.c 文件
   size_t __stdout_write(FILE *f, const unsigned char *buf, size_t len)
   {
   ...
   	return __stdio_write(f, buf, len);
   }
   
   
   ```

4. **调用 `chcore_stdout_write`**: 

   在`__stdout_write`函数的实现中，有下列传入`SYS_writev`的系统调用

   ```c
   		cnt = syscall(SYS_writev, f->fd, iov, iovcnt);
   
   ```

   其中根据头文件可知，该syscall会调用`__syscall3()` 函数，在 `syscall_dispatcher.c`文件中, 可以查找到syscall3, 然后再经历

   `__syscall6()` --*(由`SYS_writev`)* -->`chcore_writev()` -->`chcore_write()`-->`write()`

   最后的write函数,实际上查阅 `fd.h` 即可发现, 是`fd_ops`结构体中的一个field, 其实现正是 `chcore_stdout_write()` ,

   ```c
   
   static int chcore_stdout_write(...)
   {
   ...
   }
   
   struct fd_ops stdout_ops = {
   ...
           .write = chcore_stdout_write,
   ...
   };
   ```

   综上, 调用链查找完毕



## 练习题8

本题中使用 chcore_syscall6 进行系统调用，复写 musl-libc 中的代码

具体详见源代码



## 练习题9

chcore所用的c标准库实现完成后，我们可以使用chcore的编译工具链以及编译二进制文件，

指定编译工具后可以输出`.bin` 文件，放入 ramdisk 文件夹后就可以被一起加载到内核镜像中运行











