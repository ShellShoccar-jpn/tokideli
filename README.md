<p align="center">
  <img src="https://raw.githubusercontent.com/ShellShoccar-jpn/tokideli-assets/main/logo.svg" width="520" alt="tokideli">
</p>

<p align="center">
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/badge/license-public_domain-blue"></a>
  <img alt="Tested on 7 platforms" src="https://img.shields.io/badge/tested_on-Linux_Solaris_FreeBSD_NetBSD_OpenBSD_macOS_Android-informational">
</p>

# TOKI-DELI

A collection of lightweight POSIX-compliant commands for accurate time management that provides the perfect timing even for shell scripts.
"We deliver the accurate 'TOKI' (meaning "Timing" in Japanese) to your UNIX terminal and shell scripts!"

（日本語版は[こちら](README.ja.md)）

## Table of Contents

* [What is this?](#what-is-this)
* [Highlights](#highlights)
* [How to Build and Install](#how-to-build-and-install)
* [Author / License](#author--license)

## What is this?

This is a command collection to make your UNIX life more convenient! On the current POSIX commands, are you satisfied for timing management? Unfortunately, we aren't. That is because POSIX doesn't release the commands for controling time more accurately and/or more precisely than one second. For instance, image a scene you have to send text data at one second interval accurately. How can you do that with shell script? You can probably do only like that.

```sh:
cat /PATH/TO/textdata_source |
while IFS= read -r line; do
  printf '%s\n' "$line"
  sleep 1
done
```

However, that is not accurate because extra time is required to execute `while` ~ `done` sentence and `printf`. So, we made some commands to solve such problems.

To solve the above problem, you can use `valve` command by the following.

```sh:
$ cat /PATH/TO/textdata_source | valve -l 1s
```

Not only we can solve the problem but also we can make the shell script simpler! Here's an actual recording comparing a naive `while`/`sleep` loop against `valve -l`, side by side. The naive loop (left) drifts by about +0.27s over 10 iterations, while `valve` (right) stays within about +0.02s.

<p align="center">
  <video src="https://raw.githubusercontent.com/ShellShoccar-jpn/tokideli-assets/main/demo.mp4" controls muted playsinline width="520">naive while/sleep loop drifts by +0.27s over 10 iterations, while valve -l stays within +0.02s</video><br>
  <sub>Script used to reproduce this recording: <a href="https://github.com/ShellShoccar-jpn/tokideli-assets/blob/main/demo.sh">demo.sh</a> / <a href="https://github.com/ShellShoccar-jpn/tokideli-assets/blob/main/demo.tape">demo.tape</a> (the <a href="https://github.com/charmbracelet/vhs">vhs</a> recording recipe)</sub>
</p>

Several more commands are available.

| Command | Description |
|---|---|
| [`calclock`](manual/calclock.man.en.md) | Convert bewteen the Calendar time and UNIX time |
| [`delay`](manual/delay.man.en.md) | Delay each byte arriving from the standard input by a fixed amount of time |
| [`getfilets`](manual/getfilets.man.en.md) | Display timestamps (mtime, ctime, atime) of a file |
| [`herewego`](manual/herewego.man.en.md) | Sleep Until a Nice Round Time and Tell the Time |
| [`linets`](manual/linets.man.en.md) | Add timestamp to every line of text data |
| [`oobleck`](manual/oobleck.man.en.md) | Output Lines Only When the Next Line Does Not Arrive for a While |
| [`ptw`](manual/ptw.man.en.md) | A command wrapper to prevent a command from full-buffering (alternative of [stdbuf](https://www.gnu.org/software/coreutils/manual/html_node/stdbuf-invocation.html#stdbuf-invocation), see [this](manual/ptw.info.en.md) for details) |
| [`qvalve`](manual/qvalve.man.en.md) | Quantitative Valve for the UNIX Pipeline |
| [`relval`](manual/relval.man.en.md) | Limit the Flow Rate of the UNIX Pipeline Like a Relief Valve |
| [`sleep`](manual/sleep.man.en.md) | Sleep command which supports sleeping during less than a second (POSIX compliant) |
| [`surgetk`](manual/surgetk.man.en.md) | Absorb a temporary burst on the standard input into a buffer, like a surge tank |
| [`tscat`](manual/tscat.man.en.md) | Output each line at the data and time which is written in the top of the line |
| [`tshead`](manual/tshead.man.en.md) | Cut out lines up to a given time, based on the timestamp in each line |
| [`tstail`](manual/tstail.man.en.md) | Cut out lines from a given time onward, based on the timestamp in each line |
| [`typeliner`](manual/typeliner.man.en.md) | Make a Line of a Bunch of Key Types |
| [`valve`](manual/valve.man.en.md) | Adjust the Data Transfer Rate in the UNIX Pipeline |
| [`waitill`](manual/waitill.man.en.md) | Sleep until a deadline (a point in time) instead of for a duration |

To see the usages for the commands, build the command and run them with the option `--help`. For more in-depth articles on how to combine these commands and the design background, see the [`manual/`](manual/) directory (available in both English and Japanese).

## Highlights

* **Nanosecond precision** — record and compare byte- and line-level arrival times with nanosecond precision.
* **Verified on 7 operating systems** — built and tested on real Linux, Solaris, FreeBSD, NetBSD, OpenBSD, macOS, and Android machines. The C sources conform to POSIX.1-2008 and absorb each OS's quirks.
* **Zero dependencies** — one command, one self-contained C source file. No external libraries required to build.
* **Public domain** — use it under CC0 or the Unlicense, whichever you prefer.
* **Thorough bilingual documentation** — a manual for every one of the 17 commands, plus in-depth articles explaining how to combine them and why they're designed the way they are.

## How to Build and Install

First, `git clone` this repository. Then, run the `INSTALLIN.sh` with specifying the install directory. Building and installation progress interactively.

To short, all you have to do is to type the following commands. ("/usr/local/tokideli" is a typical directory for installation)

```sh:
$ git clone https://github.com/ShellShoccar-jpn/tokideli.git
$ su
# tokideli/INSTALLIN.sh /usr/local/tokideli
```

Or type the following if you want to install this in your home directoy instead.

```sh:
$ git clone https://github.com/ShellShoccar-jpn/tokideli.git
$ tokideli/INSTALLIN.sh $HOME/tokideli
```

You can add the install directory into the environment variable "PATH" by using "INSTALL.sh." Of course, you can do that manually, too.

## Author / License

ShellShoccar Japan, no rights reserved.

Everything in this repository is completely free for everyone. If you want any license to use them by all means, we'll give you [CC0](https://creativecommons.org/share-your-work/public-domain/cc0) or [the Unlicense](https://unlicense.org/). Anyway, take them freely as you like.
