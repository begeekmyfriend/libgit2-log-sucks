libgit2-log-sucks
=================

This is sample code showing how it sucks that the speed of log command of libgit2.

Test
----

Download `git` repository into `/tmp`.

```shell
        cd /tmp
        git clone git@github.com:git/git.git
```

Install libgit2 into `/usr/local/lib`. See [libgit2](https://github.com/libgit2/libgit2).

Build and run the test.

```shell
        cd libgit2-log-sucks
        make
        ./show-last-commit.sh
```

And then enjoy your time!
