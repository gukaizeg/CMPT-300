# Guide to Adding a System Call to Linux 5.4.109

This document guides the user through:

* Adding a new system call (syscall) to Linux (as of 5.4.109)
* Writing a user-level program to call it.

It is assumed that the user has downloaded the Linux kernel source code, compiled it, and is able to
boot the kernel. See [Custom Kernel Guide](CUSTOM_KERNEL_GUIDE.md) first.

## Adding a HelloWorld System Call

This guide assumes you have the Linux source code (for version 5.4.109, or more recent may also
work) in a directory linux-5.4.109/.

* Change to the `linux-5.4.109` directory:

  ```bash
  $ cd linux-5.4.109
  ```

* Create a new directory named `cmpt300/` inside `linux-5.4.109/` and change to it:

  ```bash
  $ mkdir cmpt300
  $ cd cmpt300
  ```

* Create a new file which implements your new system call:

  ```bash
  $ touch cmpt300_test.c
  ```

  * Set file contents to (using an editor like `nano`, `vim`, or `emacs`):

    ```bash
    #include <linux/kernel.h>
    #include <linux/syscalls.h>

    // Implement a HelloWorld system call
    // Argument is passed from call in user space.
    SYSCALL_DEFINE1(cmpt300_test, int, argument)
    {
      long result = 0;
      printk("Hello World!\n");
      printk("--syscall argument %d\n", argument);
      result = argument + 1;
      printk("--returning %d + 1 = %ld\n", argument, result);
      return result;
    }
    ```

  * This code will be compiled as part of the kernel, not as a user level program. It will run with
    the privilege of the kernel and without the support of the standard C library.
  * `printk()` is the kernel's version of `printf()`. It has limited formatting capabilities
    compared to `printf()`.
  * The kernel is complied using the C90 standard, not C99. Therefore you must declare all your
    variables at the top of a block (such as your function) instead of in the middle (as permitted
    in C99). Hint: Always initialize your variables to some value!
  * The `SYSCALL_DEFINE1` macro is the standard way to define a syscall in the kernel. The "1" in
    the macro means we have one parameter for this syscall. There are more variants that allow you
    to support more parameters, including `SYSCALL_DEFINE2`, `SYSCALL_DEFINE3`, and
    `SYSCALL_DEFINE4`, which corresponding support 2, 3, and 4 parameters.

* Create a header file that declares the new system call:

  ```bash
  $ touch cmpt300_test.h
  ```

  * Set file contents to:
  
    ```bash
    asmlinkage long sys_cmpt300_test(int argument);
    ```

  * Notice how the function name `sys_cmpt300_test` matches the defined syscall name previously
    (`cmpt300_test` without `sys_`).

* Create a `Makefile` to allow your new system call file to be compiled by the kernel.

  ```bash
  $ touch Makefile
  ```

  * Set file contents to:
  
    ```bash
    obj-y := cmpt300_test.o
    ```

  * If adding additional `.c` files later, you can space separate them, such as:
  
    ```bash
    obj-y := cmpt300_test.o mytest.o otherthing.o
    ```

* Integrate your new directory into the overall kernel build process by editing the Linux kernel's main `Makefile` (in `linux-5.4.109/`):
  * Find the line which defines `core-y` (near line ~1039)

    ```bash
    ifeq ($(KBUILD_EXTMOD),)
    core-y += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/
    ```

  * Add your new directory to the end of the `core-y` define:
  
    ```bash
    ifeq ($(KBUILD_EXTMOD),)
    core-y += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ cmpt300/
    ```

  * Later when you make the kernel, it will also build the contents of your `cmpt300/` directory.

* Create the new syscall by adding it to the kernel's list of system calls.
  * Open `arch/x86/entry/syscalls/syscall_64.tbl` which creates all the syscalls for the x86-64 bit
    architecture:
  * Add the following line at the end of the first block of syscall defines (i.e., after the call
    numbered ~360, and before the second block defining calls in the 512+ range).

    ```bash
    436 common cmpt300_test __x64_sys_cmpt300_test
    ```

    * The number on the left is the syscall number you are creating. This syscall will exist only in
      your kernel. In general, once a syscall is added to the mainline kernel that number is never
      reused because it would break any existing application which depends on it. For this guide,
      all that is important is that the number you select is not used by other syscalls (above it).
    * `common` defines the application binary interface.
    * `cmpt300_test` is name of the syscall, as will be listed in the header files in the kernel.
    * `sys_cmpt300_test` is the name of the function (the "entry point") which be called to service
      this syscall. This matches the name of the function created in `cmpt300_test.c`.

* Rebuild the kernel. The compiled kernel will now include your custom syscall! Now all you have to
  do is write some code which calls it (next section).

  ```bash
  $ make -j4
  ```

## Calling your System Call

### Creating a Test Application

* Under the repo root directory (i.e., one level above the `linux-5.4.109/` directory), create a new
  directory for your user-level test application:

  ```bash
  $ mkdir test-syscall
  $ cd test-syscall
  ```

* Create a test application source file:

  ```bash
  $ touch cmpt300_testapp.c
  ```

  * Set contents to:
  
    ```bash
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/syscall.h>
    
    #define _CMPT300_TEST_ 436 // for a 64 bit system

    int main(int argc, char *argv[])
    {
      printf("\nDiving to kernel level\n\n");
      int result = syscall(_CMPT300_TEST_, 12345);
      printf("\nRising to user level w/ result = %d\n\n", result);
      return 0;
    }
    ```

  * `_CMPT300_TEST_` is defined to be the syscall number we created. Normally this would be imported
    in the `sys/syscall.h` file. However, since we are building a custom kernel, we can `#define`
    our custom syscall number and not have to worry about updating `.h` files.

* Compile your test application:

  ```bash
  $ gcc -std=c99 -D _GNU_SOURCE -static cmpt300_testapp.c -o cmpt300_testapp
  ```

  * `-D_GNU_SOURCE` allows access to the `syscall()` function defined in `unistd.h`.
  * `-static` causes all necessary library functions to be statically linked into the binary,
    thereby freeing us from having to ensure all required libraries are in the virtual machine.
  * This should produce an executable `cmpt300_testapp`.
  * Running this application on a normal kernel will have the syscall do nothing (return -1).

    ```bash
    $ ./cmpt300_testapp

    Diving to kernel level

    Rising to user level w/ result = -1
    ```

  * However, running it on the custom kernel (next section) will call your kernel code.

### Running a Test Application

* Boot your custom kernel using QEMU (as per the [Custom Kernel Guide](CUSTOM_KERNEL_GUIDE.md)).
* Transfer your `cmpt300_testapp` executable to your virtual machine (as per the [Custom Kernel
  Guide](CUSTOM_KERNEL_GUIDE.md)). Command is likely:

  ```bash
  $ scp -P 9347 cmpt300_testapp ubuntu@localhost:
  ```

* In your virtual machine, check that `cmpt300_testapp` has been transferred:

  ```bash
  $ ls
  ```

  * You should see `cmpt300_testapp` in the listing.

* In your virtual machine, run the test application. You should see the following:

  ```bash
  ubuntu@test3:~$ ./cmpt300_testapp
  Diving to kernel level

  Rising to user level w/ result = 12346
  ubuntu@test3:~$
  ```

  Then check the "real" output in kernel message, issue:
  
  ```bash
  $ dmesg
  ```

  At the end of the output, you should see something like:

  ```bash
  [  186.746037] Hello World!
  [  186.751607] --syscall argument 12345
  [  186.751938] --returning 12345 + 1 = 12346
  ```

  * If you see this, then congratulations! You have now written, complied, and finally called some
    kernel code! If not, see the troubleshooting section below.

* Troubleshooting
  * If your application returns the result -1 while running in your virtual machine, try the
    following:
    * Make sure your `cmpt300_testapp.c` file defines the system call number to match the number you
      entered in `syscall_64.tbl`.
    * Re-transfer your `cmpt300_testapp` executable to your virtual machine and rerun the command.
      (This may not be the problem, bit it's fast to try!)
    * Ensure your custom kernel compiled correctly, and that you booted the correct kernel. You
      could check this by changing the "Local version" of the kernel (see the [Custom Kernel
      Guide](CUSTOM_KERNEL_GUIDE.md)), rebuild the kernel, and relaunch QEMU. Then check that your
      VM is running that new custom kernel. Display the kernel version using:
  
      ```bash
      $ uname -a
      ```

    * Ensure you are running the correct version of the OS with the user-level code.
      * Both host OS and QEMU should have same architecture (number of bits):
        * On the host, check if it's 64 bit (x86_64) with:

          ```bash
          $ uname -m
          ```

        * Repeat the test on the target OS (QEMU):

          ```bash
          $ uname -m
          ```

        * The user-level test code must be configured with the correct syscall number matching the
          value you put in `syscall_64.tbl`.
  * If your syscalls always fails (return -1), double check the following:
    * You have downloaded your latest version of your test application.
    * You have correctly added the syscall number to the `syscall_64.tbl` file.
    * Your syscall numbers match in your application to `syscall_64.tbl`.
    * You have successfully recompiled your kernel code. To prove you are compiling the code,
      temporarily make an error in your kernel code implementing the syscall and recompile the
      kernel. If the build fails on your code then you know it is being compiled; if not, you have a
      problem with your make files. After ensuring this, remove the error.

## Acknowledgements

Created by Brian Fraser, Arrvindh Shriraman, and Tianzheng Wang. Modified by Steve Ko.
