# gtodo

一个用GTK+编写的待办应用。

编译：

```bash
gcc `pkg-config --cflags gtk+-3.0` -o gtodo app.c `pkg-config --libs gtk+-3.0`
./gtodo
```

或用 meson 进行编译：

```bash
meson builddir
cd builddir
ninja
./gtodo
```
