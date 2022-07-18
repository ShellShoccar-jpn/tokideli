# What Is PTW Command for?

By this document, you will know what `ptw` command is for.

（日本語版は[こちら](ptw.info.ja.md)）


## Limitation of `stdbuf` Command

Try the following one-liner commands.

```sh:
# (A) for macOS users
$ while sleep 1; do date; done | stdbuf -o L tr 1 1 | cat

# (B) for Python3 users
$ while sleep 1; do date; done | stdbuf -o L python3 -c 'import sys; [print(l,end="") for l in sys.stdin]' | cat

# (C) for Perl users
$ while sleep 1; do date; done | stdbuf -o L perl -e 'while(<>){print}' | cat
```

The time notification will come out every second if `stdbuf` command works correctly, but it will not probably work. That is because `stdbuf` command fails to change the buffering mode of the target commands from the full-buffering (default) to the line-buffering. They ignore the request from `stdbuf` and buffers the time notification data until its buffer is full. As a result, the time notification does not come on time.

### Why Does the Problem Occur?

To understand the problem, you should know how `stdbuf` works.

`stdbuf` use the "LD_PRELOAD" mechanism to change the buffering mode of the target command. Most UNIX commands are built with dynamic link libraries not to raise their filesizes. So we can modify their behaviors if we can give them another dynamic library while they are reading libraries.

The "LD_PRELOAD" is the mechanism to do so. This is a good method to customize the command behavior. `stdbuf` sends the program to the target command to change the buffering mode with the "LD_PRELOAD", and makes the target command execute the program before their beginning.

However, the mechanism has no effect on the commands built with static link libraries, therefore. The `tr` command on macOS is one such command. In this case, you can, fortunately, avoid the problem by using the `gtr` command, produced by GNU, instead of the original `tr`. But GNU does not always give us substitute commands for all static link commands.

In the (B) and (C) cases, it is slightly different. Both Python and Perl have their own buffering management routine. So they reset the buffering mode to the full-mode after booting. If you want to solve the problem of these languages, you have to write the statement for changing the buffering mode on their scripts.

## Is There Any Solution for the Problem?

Yes, we discovered one. Firstly, try again with the following one-liner.

```sh:
# (A) for macOS users
$ while sleep 1; do date; done | tr 1 1

# (B) for Python3 users
$ while sleep 1; do date; done | python3 -c 'import sys; [print(l,end="") for l in sys.stdin]'

# (C) for Perl users
$ while sleep 1; do date; done | perl -e 'while(<>){print}'
```

This one-liner will work correctly. That phenomenon is the clue to solve the problem.

According to C language specifications, programs whose STDOUT are connected to a terminal should set the buffering mode to the line-buffering instead of the full-buffering. In the above case, the `tr` command's STDOUT is connected to the terminal because it is located at the last of the one-liner. Therefore, we can change the buffering mode of the target command by showing a terminal to it, not depending on the LD_PRELOAD. In the (B) and (C) cases, the languages follow the C language specs, and they decide the mode whether connected to a terminal or not.

### Pseudo terminal (PTY) Is Good for the Purpose

The `ptw` command we released is a wrapper command to pretend to be a terminal (neither a pipeline nor a file) and cheat the target command. The steps to work when you execute `ptw TARGET_COMMAND ARG1 ARG2 ...` is the following:

1. `ptw` starts instead of TARGET_COMMAND.
1. `ptw` creates a PTY.
1. `ptw` also creates a child process with the fork() function. So, the parent and the child can share the PTY each other.
1. The child replaces the STDOUT endpoint with the PTY's descriptor.
1. The child finally changes itself to `TARGET_COMMAND ARG1 ARG ...` with the exec() function. By this trick, the TARGET_COMMAND thinks being connected to a terminal and changes the buffering mode. And it unconsciously sends data to the PTY instead of the actual STDOUT.
1. The parent enters the infinite loop to receive the data the TARGET_COMMAND sent via the PTY. Then, the parent transfers them to the actual STDOUT whenever new data come.

So you can solve the problem by typing the following one-liner.

```sh:
# (A) for macOS users
$ while sleep 1; do date; done | ptw tr 1 1 | cat

# (B) for Python3 users
$ while sleep 1; do date; done | ptw python3 -c 'import sys; [print(l,end="") for l in sys.stdin]' | cat

# (C) for Perl users
$ while sleep 1; do date; done | ptw perl perl -e 'while(<>){print}' | cat
```

Thus, **you can think of the `ptw` command as a more strongly effective version of `stdbuf -o L`.**

## Usage of PTW Command

It is very easy. Just insert the word "ptw" before the target command. If you have used "stdbuf -o L," replace the phrase with the word "ptw." That's all.

But you do not have to add "ptw" word for the last command in the one-liner. If you do so, `ptw` command just executes the target command.

```sh:
ptw CMD1 arg1a arg1b ... | ptw CMD2 arg2a arg2b ... | ... | CMD9 arg9a arg9b ...
```

## But PTW Is Not Incvincible...

There is no doubt that `ptw` effectiveness is stronger than `stdbuf`. However, there are still a few commands that `ptw` cannot change the buffering mode.

### `mawk`

`mawk` is one of imprementations of AWK. Some versions of Raspberry Pi OS and Ubuntu adopt it as a default AWK. `mawk` also manages the buffering mode itself and overwrites the preset mode no matter what program sets the mode in advance.

There is no choice but to use the `-W interactive` option instead of `ptw` command if the AWK you are using could be mawk. So we suggest you write the following paragraph at the beginning of your shell script instead of adding the "ptw" word for every AWK.

```sh:
# (at the beginning of your shell script)
case $(awk -W interactive 'BEGIN{print}' 2>&1 >/dev/null) in
  '') alias awk='awk -W interactive';;
   *) alias awk='ptw awk'           ;;
esac
```

### Perl, Python with the Statement for Specifying the Buffering Mode

In these cases, they also overwrite the preset buffering mode. There is no choice but to remove the statement.


### Other Commands Overwriting the Buffering Mode

It is the same reason. There is no choice but to remove the cause of deciding the buffering mode.

### Commands Written in Shell Script

The dominating program of the buffering mode is not the shell script itself but each command in the shell script. So you have to add the "ptw" word to them directly.

## Side Effects

Some commands change their behaviors depending on whether they are connected to a terminal or not. So you might get undesired data because the commands wrapped by `ptw` think of being connected to a terminal.

For instance, some implementations of the grep commands color their output text data when they are connected to a terminal. So you may be able to see colored text data by the following command.

```sh:
$ seq 1 100 | ptw grep --color=auto 1 | cat
```

However, you are rarely troubled by options like `--color = auto`. That is because these options are usually defined by the UNIX shell alias. And external commands, including `ptw`, cannot recognize the alias definitions.
