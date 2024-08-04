# TOKI-DELI

We will deliver the "TOKI" (means "Timing" in Japanese) management command to your UNIX computer!

（日本語版は[こちら](README.ja.md)）

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

Not only we can solve the problem but also we can make the shell script simpler!

Several more commands are available.

* [`calclock`](bin/calclock) ..... Convert bewteen the Calendar time and UNIX time
* [`getfilets`](c_src/getfilets.c) Display timestamps (mtime, ctime, atime) of a file
* [`herewego`](c_src/herewego.c) . Sleep Until a Nice Round Time and Tell the Time
* [`linets`](c_src/linets.c) ..... Add timestamp to every line of text data
* [`ptw`](c_src/ptw.c) ........... A command wrapper to prevent a command from full-buffering (alternative of [stdbuf](https://www.gnu.org/software/coreutils/manual/html_node/stdbuf-invocation.html#stdbuf-invocation), see [this](https://github.com/ShellShoccar-jpn/tokideli/blob/main/manual/ptw.info.en.md) for details)
* [`relval`](c_src/relval.c) ..... Limit the Flow Rate of the UNIX Pipeline Like a Relief Valve
* [`sleep`](c_src/sleep.c) ....... Sleep command which supports sleeping during less than a second (POSIX compliant)
* [`tscat`](c_src/tscat.c) ....... Output each line at the data and time which is written in the top of the line
* [`typeliner`](c_src/typeliner.c) Make a Line of a Bunch of Key Types
* [`valve`](c_src/valve.c) ....... Adjust the Data Transfer Rate in the UNIX Pipeline

To see the usages for the commands, build the command and run them with the option `--help`.

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