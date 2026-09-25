# PTW(1)

（日本語版は[こちら](ptw.man.ja.md)）

## Name

ptw - Pseudo-terminal wrapper (releases full buffering)

## Synopsis

```sh:
ptw [-f] command [argument [...]]
```

## Description

ptw is a kind of command wrapper: for the command executed as `command [argument [...]]`, it makes the standard output look as if it were connected to a terminal. Most commands, following the ANSI C standard, switch their standard output from full-buffering mode to line-buffering mode when they are actually connected to a terminal — even if they are actually sitting in the middle of a command pipeline.

The stdbuf(1) command produces a similar effect, and as far as buffering mode is concerned, ptw is expected to give the same result as running `stdbuf -o L command [argument [...]]`. However, this command is more powerful than stdbuf(1) and works on more commands than stdbuf(1) does. In fact, since stdbuf(1) relies on the LD_PRELOAD mechanism, it has no effect on statically linked commands, nor on language commands such as Perl or Python that manage their own buffering mode internally. The ptw command, however, is effective against most such commands as well.

To describe more concretely what is meant by "making it look as if it were connected to a terminal," ptw performs the following steps.

1. ptw starts up in place of the target command.
1. ptw creates a pseudo-terminal.
1. ptw creates a child process with fork(2). This allows the pseudo-terminal to be shared between the parent and the child.
1. The child process reconnects its own standard output to the descriptor (connection point) of the pseudo-terminal.
1. The child process execs(2) into `command [argument [...]]`. The command then judges that it itself is connected to a terminal and changes its buffering mode, and it writes its data not to the real standard output but to the descriptor of the pseudo-terminal prepared by the parent process.
1. The parent process enters an infinite loop watching the descriptor (connection point) of the pseudo-terminal. Whenever data arrives from it, the parent writes it straight to the real standard output.

## Options

### -f

Connects a pseudo-terminal even when it is not actually necessary to connect one to the standard output of the wrapped command — specifically, when the wrapped command sits at the very end of the command pipeline. In that case, the command is already connected to a real terminal, so there is no need to reconnect it to a pseudo-terminal. In such a case, the ptw command would normally not create a pseudo-terminal at all, and would simply exec(2) the target command. However, when this option is given, ptw always reconnects to a pseudo-terminal regardless of whether it is actually necessary. This may be useful when you want to check how this command behaves.

## Return Value

In principle, the return value of the wrapped command is returned as is. However, this command may itself return a value of 1 or greater, for instance when the arguments are invalid or when it fails to start the wrapped command.

## Examples

Reconnect the standard output of the perl command to a pseudo-terminal to encourage it to run in line-buffering mode. (If ptw were not used here, perl would run in full-buffering mode, and the current time would not be displayed on the screen in real time.)

```sh:
$ while sleep 1; do date; done | ptw perl -e 'while(<>){print}' | cat
```

## Bugs

### Commands Whose Full Buffering Cannot Be Released

The ptw command has no effect on commands that are fixed (reconfigured internally) to full-buffering mode. mawk(1) is known to be such a command, and it may be the AWK implementation used by Linux distributions such as Debian or Ubuntu.

If the awk command you are using might be mawk, it is recommended to put a definition such as the following at the beginning of your shell script.

```sh:
# (Specify this at the beginning of the shell script)
case $(awk -W interactive 'BEGIN{print}' 2>&1 >/dev/null) in
  '') alias awk='awk -W interactive';;
   *) alias awk='ptw awk'           ;;
esac
```

Also, this command has no effect on commands implemented as shell scripts. In that case, wrap the commands actually executed inside the shell script directly with this command instead.

### Side Effects from Commands That Change Behavior Depending on Whether They Are Connected to a Terminal

Some commands change their output depending on whether they are connected to a terminal or not. Since the target command wrapped by this command is led to believe it is connected to a terminal, you may get unintended output.

The ls(1) command is one such example; in the following example, the result differs depending on whether it is wrapped with ptw or not.

```sh:
$ ls | cat
$ ptw ls | cat
```

When ptw is attached, `ls` tries to arrange the filenames side by side according to the terminal width, whereas without it, it outputs one filename per line. So, if you want one filename per line even when ptw is attached, use the -1 option.

```sh:
$ ptw ls -1 | cat
```

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").

## See Also

[What Is PTW Command for?](ptw.info.en.md)
