# TOKI-DELI

We will deliver the "TOKI" (means "Timing" in Japanese) management command to your UNIX computer!

## What is this?

This is a command collection to make your UNIX life more convenient! On the current POSIX commands, are you satisfied for timing management? Unfortunately, we aren't. That is because POSIX doesn't release the commands for controling time accurately and/or precisely. For instance, image a scene you have to send text data at one second interval accurately. How can you do that? You can probably do only like that.

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

Several more commands are available.

* [`calclock`](bin/calclock) ..... Convert bewteen the Calendar time and UNIX time
* [`getfilets`](c_src/getfilets.c) Display timestamps (mtime, ctime, atime) of a file
* [`herewego`](c_src/herewego.c) . Sleep Until a Nice Round Time and Tell the Time
* [`linets`](c_src/linets.c) ..... Add timestamp to every line of text data
* [`ptw`](c_src/ptw.c) ........... A command wrapper to prevent a command from full-buffering (alternative of [stdbuf](https://www.gnu.org/software/coreutils/manual/html_node/stdbuf-invocation.html#stdbuf-invocation)
* [`sleep`](c_src/sleep.c) ....... Sleep command which supports sleeping during less than a second
* [`tscat`](c_src/tscat.c) ....... Output each line at the data and time which is written in the top of the line
* [`valve`](c_src/valve.c) ....... Output each byte/line at the specified interval

To get the information for the commands, build the command and run them with the option `--help`.

## How to Build

First, `git clone` this repository. Then, run the `MAKE.sh` command to build the command with the source files.

To short, all you have to do is to type the following commands.

```sh:
$ git clone https://github.com/ShellShoccar-jpn/tokideli.git
$ su
# tokideli/INSTALL.sh /usr/local/tokideli
```

Or type the following if you want to install this in your home directoy instead.

```sh:
$ git clone https://github.com/ShellShoccar-jpn/tokideli.git
$ tokideli/INSTALL.sh $HOME/tokideli
```

The "INSTALL.sh" will build commands, copy the command to the directory, and help you add the directory into the environment variable "PATH."

## Author / License

ShellShoccar Japan, no rights reserved.

Everything in this repository is completely free for everyone. If you want any license to use them by all means, we'll give you [CC0](https://creativecommons.org/share-your-work/public-domain/cc0) or [the Unlicense](https://unlicense.org/). Anyway, take them freely as you like.