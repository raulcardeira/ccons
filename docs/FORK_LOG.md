# `ccons` fork by Raoul Abdullah

## DAY 0: 2023/10/31

Upon starting to learn C, one of my first searches was for a way to experiment with C code in an interactive way, with a REPL-like experience. Even though C is a compiled language, I knew about efforts in the Rust community for a similar experience for Rust (the now no-longer developed `rusti` project), so, I thought if it would be possible to have a similar experience in C, as it would be easier to test small syntax details.

I then stumbled upon [`ccons`](https://github.com/asvitkine/ccons), which aims to offer such capability. I decided to try this tool, as I was determined to find the experience I was looking for. Luckily, `ccons` features a presentation that goes over some of its architecture, and the installation instructions seemed easy to follow. It made use of `LLVM` as well, with was a tool that I was curious about for a long time already.

The first step was placing the `ccons` directory, obtained from the GitHub repository, in the `llvm/tools` directory. I then firstly thought that if I already had `llvm` installation in my system, I could find its `tools` directory and place the `ccons` directory there, without requiring me to build the entire `LLVM` toolchain. But I couldn't find the `tools` directory, albeit I didn't put much effort into it, to be honest. I decided instead to build the `LLVM` toolchain. Following the instructions on the [`LLVM`](http://clang.llvm.org/get_started.html) website, I made a "shallow" clone of the `LLVM` repository. I then proceeded to build `LLVM`. I was aware that this process would take some time, but it ended up taking about 8 hours. I had went out of home and when I got back proceeded to test the new `LLVM` installation. In this process, I learned that using `clang --version`, one gets the information on the directory from which `clang` is being called. With this, I learned that I had to use `./clang --version` instead of `clang --version` (in the `/build/bin` directory in the built `LLVM`), or the system would call the `clang` installation from the package manager. But I was able to successfully verify that I had built `LLVM` correctly, which was somewhat of a win. 

At this point I had also studied how to install `editline`, the library required by `ccons` to provide the terminal line-editing interface. Once I realized that there were no "release" distributions of this library, but I just had to follow the instructions in the README (which was just a `./configure` and `make`, followed by `make install` to make the library (or program) available to the system -- which I'm finding to be somewhat of a standard among the C community).

Once this was done, I moved on to building `ccons` itself. Unfortunately, when running the `make` command, I encountered an error:

```
[  6%] Building CXX object CMakeFiles/ccons.dir/ccons.cpp.o
/mnt/c/Users/cwolf/Desktop/raul/c/llvm-project/build/tools/ccons/ccons.cpp:14:10: fatal error: llvm/ADT/OwningPtr.h: No such file or directory
   14 | #include <llvm/ADT/OwningPtr.h>
      |          ^~~~~~~~~~~~~~~~~~~~~~
compilation terminated.
make[2]: *** [CMakeFiles/ccons.dir/build.make:76: CMakeFiles/ccons.dir/ccons.cpp.o] Error 1
make[1]: *** [CMakeFiles/Makefile2:83: CMakeFiles/ccons.dir/all] Error 2
make: *** [Makefile:136: all] Error 2
```

Initially I was a bit dismayed because of the unsuccessful build. But I studied the situation further, and after some time, discovered that it was a case of the `OwningPtr.h` header file, not being found by the compiler. After some more study, I discovered that this was because I had made a mistake in my installation of `LLVM`. That was, by making a "shallow" copy of the `LLVM` repository, I only obtained the most recent commit. So, I build the most recent version of `LLVM` (which is version 18). And, the fact is that `ccons` was developed a considerable amount of time ago. So, I dug further and found out that the `OwningPtr` type got deprecated from `LLVM` a long time ago as well. 

So, now, I have two things that I want to do. The first one is to build a version of `LLVM` that can be used to build `ccons`, so that I can try it. After that, and if I'm successful, I also plan to try to patch `ccons` so that it can use a more recent version of `LLVM`. I don't know what that will entail, but I think that it can prove a useful and fun learning experience. 

---

## DAY 1: 2023/11/02

Yesterday was an Holiday here in Portugal, so I didn't work on this project. So, let's begin.

The first goal is to have `ccons` running on my system. For that, I need to know what version of `LLVM` it was built with. After some research, I found that the `OwningPtr` type got deprecated from `LLVM` code after version 3.0. So, I decided to try to get this version.

I tried looking around in the `llvm-project` GitHub repository, to see if I could download the 3.0 release directly from GitHub. But I wasn't able to find the option to do so (I'm still a bit novice at GitHub).

Now, because in my system I only had a "shallow" clone of the `LLVM` repository, it only contained the most recent commit. In order to get the project history, I needed to convert my clone into a "full" clone, with `git fetch --unshallow`.

When this was done, I needed to find the specific commit with the 3.0 release tag to checkout (I had seen in GitHub that the LLVM developers kindly provide release tags for their releases). Unfortunately, when I used `git tag`, it only showed me tags until version 10.0. This is somewhat disappointing as I thought I had cloned the entire project history.

At this point, I noticed a line right on top of the `LLVM` "Get Started" page. The "Release Clang Versions" part, where they provide a link right to the `LLVM` releases. I hadn't noticed this before, which is just a case of sometimes not knowing where to find the specific releases of software. I also hadn't take even 10 minutes to browse a bit through `LLVM`'s website, to know its history and so on, which could have been useful.

Also, at this point, I remember about a small but important detail which is useful for the task at hand. Which is that, I went to see in the [`ccons` presentation](https://raw.githubusercontent.com/asvitkine/ccons/master/docs/ccons-presentation.pdf), if I remembered correctly that a saw that a date in there. And there it was: Winter 2009. This is helpful in choosing what version of `LLVM` to use, as I can see from their release page the release date for each version. With this, I actually could see that the 3.0 version, which I had decided upon, was actually released on December 1st 2011, which is later than the release date of `ccons`. But, I still decided to try to use this version, as I knew it contained the `OwningPtr.h` header file. On a personal note I was very happy with this bit of investigation.

Upon extracting the `llvm-3.0.tar.gz` tarball and examining its contents, I was happy to find that the project structure looked similar to what I had seen in the 18.0 version. Which included the `tools` directory, in which one is supposed to place `ccons` in. Now, I was hoping that the build instructions for this version of `LLVM` would be the similar to what I had went through, as that was straightforward. But, something was puzzling me already. Which is that, now, `clang-3.0` is  distributed in a separate directory than the `llvm-3.0` one. And I'm thinking that I will still need `clang-3.0` to compile `ccons`. At this point, I'm hoping that there are some build instructions for this version of `LLVM`, but I'm not so positive this is the case.

I went back to the `LLVM` release page for the 3.0 release, and found that they provide the "Release Notes", which contain "Installation Instructions". These were very detailed but also seemed very complex, and they mentioned the use of `llvm-gcc` to build `LLVM` 3.0. At this point, I realized that the build instructions perhaps had changed significatively in these years. But, after some research, I learned that I could use `clang` 3.0 to build `LLVM` 3.0. While not the official documented way, I decided to try this.

The first step in doing this was building `clang` 3.0 itself. If at this point I had read the README.txt in this repository, I would have realized I was doing the wrong think (I will explain soon). Upon trying to build `clang` 3.0, I immediately encountered the error:

```
cwolf_ubuntu@DESKTOP-0Q10SDT:/mnt/c/Users/cwolf/Desktop/raul/c/clang-3.0/clang-3.0.src/build$ cmake ..
-- The C compiler identification is GNU 11.3.0
-- The CXX compiler identification is GNU 11.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
CMake Deprecation Warning at CMakeLists.txt:5 (cmake_minimum_required):
  Compatibility with CMake < 2.8.12 will be removed from a future version of
  CMake.

  Update the VERSION argument <min> value or use a ...<max> suffix to tell
  CMake that the project does not need compatibility with older versions.


CMake Error at CMakeLists.txt:32 (get_filename_component):
  get_filename_component called with incorrect number of arguments


-- Configuring incomplete, errors occurred!
See also "/mnt/c/Users/cwolf/Desktop/raul/c/clang-3.0/clang-3.0.src/build/CMakeFiles/CMakeOutput.log".
```

I thought that this could be because the version of `CMake` required by `clang` 3.0 was not the version I had installed in my system (i.e., it could required an older version). But, it was at this point that I read the README.txt (or, rather, the INSTALL.txt) in the `clang` 3.0 source repository, where they explain that `clang` is supposed to be build alongside the `LLVM` build process. And they explain that one has to copy the `clang` 3.0 source repository to the `llvm/tools` directory in the `LLVM` repository. I had actually already seen this when building `LLVM` version 18.0, but I guess I didn't pay enough attention to it or had forgotten about this. This is also a great reminder to pay more attention to the README.txt of these projects, as they contain very useful information.

So, now, I proceeded to study a bit more of the "Installation Instructions" for `LLVM` 3.0, and noticed that the developers then describe the process of a "Local LLVM Configuration" for building, which includes running the `configure` script. At this point, I decided to try to run this script, with the `--prefix` option, in order to specify a target directory for the build artifacts:

```
mkdir build
./configure --prefix=$(pwd)/build
```

The script ran successfully, which left me somewhat hopeful that I could now build `LLVM` 3.0, along with `clang` 3.0. But then, after running `gmake`, I immediately get the error:

```
gmake[1]: Entering directory '/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/lib/Support'
llvm[1]: Compiling APFloat.cpp for Release build
In file included from APFloat.cpp:15:
In file included from /mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/include/llvm/ADT/APFloat.h:104:
In file included from /mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/include/llvm/ADT/APInt.h:18:
In file included from /mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/include/llvm/ADT/ArrayRef.h:13:
In file included from /mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/include/llvm/ADT/SmallVector.h:17:
/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/include/llvm/Support/type_traits.h:20:10: fatal error: 'utility' file not found
#include <utility>
         ^~~~~~~~~
1 error generated.
/usr/bin/rm: cannot remove '/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/lib/Support/Release/APFloat.d.tmp': No such file or directory
gmake[1]: *** [/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/Makefile.rules:1495: /mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/lib/Support/Release/APFloat.o] Error 1
gmake[1]: Leaving directory '/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/lib/Support'
gmake: *** [/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.0/llvm-3.0.src/Makefile.rules:782: all] Error 1
```

I studied a bit this error, and did a small test in writing a small C++ program to test if the `utility` header can be included by my system's C++ compiler. The program was just the following, and, if it compiled successfully, it meant that the C++ compiler could detect this header just fine:

```
#include <utility>

int main() {
    return 0;
}
```

I first compiled this program with `g++ -o test_utility test.cpp`, because I thought that `g++` was the compiler that the `LLVM` 3.0 `configure` script had detected. The program compiled successfully. I then tried to compiled the program with `clang` (my system's installation of `clang`). This time, I encountered the same error:

```
test.cpp:1:10: fatal error: 'utility' file not found
#include <utility>
         ^~~~~~~~~
1 error generated.
```

This leads me to think that, if I use `g++` to build `LLVM` 3.0, perhaps I can be successful. Now, it was just a matter of having the `configure` script use `g++` instead of `clang`. So, I tried looking around, first in the `Makefile`, then in the `configure` script and the `CMakeLists.txt` files, but couldn't find a clear mention of the compiler to use. But, I went again to the `LLVM` documentation, and found out that the `configure` script will use the C and C++ compilers that are declared in the `CC` and `CXX` environmnet variables, respectively. So, I did an `export CC="gcc"` and an `export CXX="g++"`, and after that ran the `configure` script again. After it ran, I then proceeded to do another `gmake`. This time, I didnt' encounter the `utility` error, but I soon found another one. This is the complete output of that compilation:

```
[ISSUE WITH COPYING C COMPILER OUTPUT BECAUSE IT IS ANSI IT SEEMS]
```


This again seems to me be an issue with the tool that I'm using to build `LLVM`. It is understandable that there can have been changes in `gcc` and `g++` during this more-than-ten-years period.

Also, there's something that's been annoying me, which is that, I don't understand how can I obtain the `llvm-gcc` compiler. The `LLVM` documentation doesn't make it clear enough to me, and I believe that it could help me in building `LLVM` 3.0, after all, that would be following their exact instructions. So, it bothers me a little bit that I'm not following the exact instructions as stated in the documentation.

At this point I took a break, and when I got back, I decided to obtain an older `gcc` to try to compile `LLVM` with it. The first step was knowing which version of `gcc` to get. After some research, I settled on version 4.2.4, which was released around that timeframe. Again, it took some work to figure out how to obtain the `gcc` version 4.2.4 (or, rather, its source code), but I finally discovered the `gcc` releases are available at (https://ftp.gnu.org/gnu/gcc/). Once I extracted it, and proceeded to run the `configure` script and then build with `make`, I initially saw things as going well, as `gcc` seemed to be compiling successfully. But, after some time, `make` exited with an error. Now, the output from the `gcc` compilation is so big and somewhat cryptic that I can't even understand what the actual error is.

With yet another attempt fluttered, I decided to look once again into the documentation for the `LLVM` 3.0 release, and found out that the version of `gcc` that is recommended to use is either 3.3.3 or 3.4.0. As by now I'm already somewhat familiar with the process of downloading a `gcc` version and trying to build it, I decided to try to build `gcc` version 3.3.3. However, I was still expecting to encounter issues, as I would still be building `gcc` 3.3.3 with my system installation of `gcc`, which is version 11.3. As I expected, this build also also failed.

At this point, I started to suffer a bit from demotivation due to all the failed build attempts, and I was also starting to feel a bit exhausted from the attempt to understand this entire process of trying to build `LLVM` version 3.0 (even though by now I thought I understood at least the basics of it). And also, it had already been two days of working on this project and I was still far away from being able to test `ccons`.

So, I decided to find another way in which I could progress in this project. And the way was simple: it was simply the correct time to ask for help. While initially I had envisioned that I would try to tackle this project on my own, for the biggest learning experience (and also a kind of test of my capabilities), I had also planned that if I find the task too demanding, I would seek help. And this is actually a really good strategy, because, as one of my goals with this project was to showcase my skills to a prospective employer, it is actually good that I can show that I can ask for help when I need it. This is a very important, often underlooked skill. In my past professional life I suffered at times from not asking for help, which resulted in a not-optimal outcome for the project I was dealing with. So, with this, I decided to create a post on the `r/C_Programming` asking if someone would be interested in providing me with guidance in this project or even become a collaborator in it.

And that ends DAY 1.

---

## DAY 2: 2023/11/04

When beginning this day, the immediate thing that I noticed is a big mistake right at the beginning of this project. The fact is that I blindly assumed that, because `ccons` is a C ecosystem tool, it was written in C. But it is not. It is almost entire written in C++. By blindly assuming it was written in C, I didn't even look at the source code files, where I could have seen that they end in `.cpp`. Also, the compiler was outputing it was compiling C++ code. 

So, at this point I had to think that, if my initial goal was to learn C, then perhaps this project is not the best for it. On the other hand, I really want to try to use `ccons`, even if just for fun. I am also learning a lot from working with these build processes. So, I decided to continue with the project, and at least try to accomplish goal 1: Just using `ccons`.

Then, I noticed another thing. I did a `git log` in the `ccons` repository, and saw that in one of the last commits, the author of the tool mentions the use of `LLVM` 3.3. So, I downloaded this release from the `LLVM` website. Unfortunately, I couldn't find the installation instructions on the release notes, like it had happened before. But, I went to check in the source directory, and found the `docs` directory, which contained a `README.txt`. In that, I learned that at this point the `LLVM` developers had moved to the `Sphinx` documentation generation system. But they also mentioned that the source files for the document, that end in the `.rst` ending, should be quite readable in source form.

I browsed the files in the `docs` folder and saw the `GettingStarted.rst` file. In there, the developers recommend the use of `gcc` 4.2. So, I will try to use this version of `gcc`. The issue, though, is that I don't know if I will be able to build `gcc` 4.2. Also, because I'm somewhat tired of trying to build these tools, I decided that I could look for a binary distribution of `gcc` instead. But, the reality is that I was unable to find one. It seems there doesn't exist some kind of repository containing all the `gcc` releases in binary format. 

At this point, I noticed that the `LLVM` developers also recommend the use of `clang` to compile `LLVM`. So, I went to search for binary distributions of `LLVM`. And, in fact, the developers provide them, in the same page as the source code release. I downloaded the binary for `clang` version 3.3, which also includes `LLVM` 3.3. Once extracted, I proceeded to try to build `LLVM` 3.3 with the binary of `clang` and `LLVM` version 3.3. To do this, I had to set up the `CC` and `CXX` environment variables for the `LLVM` `configure` script to use the binary version that I just downloaded, with the following commands:

```
cwolf_ubuntu@DESKTOP-0Q10SDT:/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.3-binary/clang+llvm-3.3-Ubuntu-13.04-x86_64-linux-gnu/bin$ export CC="/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.3-binary/clang+llvm-3.3-Ubuntu-13.04-x86_64-linux-gnu/bin/clang"
cwolf_ubuntu@DESKTOP-0Q10SDT:/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.3-binary/clang+llvm-3.3-Ubuntu-13.04-x86_64-linux-gnu/bin$ export CXX="/mnt/c/Users/cwolf/Desktop/raul/c/llvm-3.3-binary/clang+llvm-3.3-Ubuntu-13.04-x86_64-linux-gnu/bin/clang++"
```

Unfortunately, when running the `LLVM` 3.3 `configure` script, I encountered the error:

```
checking for C compiler default output file name... configure: error: C compiler cannot create executables
See `config.log' for more details.
```

I had never encountered this error before, so I didn't know what it meant. To debug it, I tried running the `clang` compiler on a simple "Hello, world" C program, but I got the error:

```
/..//bin/ld: cannot find crtbegin.o: No such file or directory
/..//bin/ld: cannot find -lgcc: No such file or directory
/..//bin/ld: cannot find -lgcc_s: No such file or directory
clang: error: linker command failed with exit code 1 (use -v to see invocation)
```

I then thought that this can be because the version of `clang` and `LLVM` I downloaded is for Ubuntu 13.04. 

At this point, I started to consider that I could set up, with Docker, an Ubuntu environment (with Ubuntu 13.04, for example), and use this environment to try to compile `ccons`. Because, along the way, I learned that I could set up `apt` to look for the version of software that I want in a specific Debian "snapshot". I would just need to find a good snapshot.

Unfortunately, when looking for Ubuntu 13.04 images on Docker, I didn't find any official images from Canonical (probably because this version of Ubuntu is not supported or recommended anymore).

Also, upon trying my system's Docker installation, I noticed that it isn't correctly set up, as I probably messed up with it (my work environment is based on Windows+WSL2, and I'm not a big fan of the Docker Desktop application for Windows).

So, at this point, I decided that I've already done a good attempt at trying to use `ccons`. Also, my requests for help weren't successful, so I am not feeling like continuing on this project completely alone. I also feel that even though I didn't achieve even the first main objective, I actually achieved some overall learning and progress in my ability as a dev. There's also this document, which for me serves as a proof-of-work that I can write good technical documentation.

So, in conclusion, I have mixed feelings for this project. While on one hand, I learned a lot, like I explained before, I still would have really liked to use `ccons`. But because I found it a somewhat hard project, it was actually a bit demanding on my capabilities and left me somewhat exhausted (mostly due to working completely on my own which can be draining on one's energies).