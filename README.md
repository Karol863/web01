A simple http server using the linux socket API.

## Building

```console
cc -o web01 main.c -march=native -O2 -flto -pipe -s -D_FORTIFY_SOURCE=1 -DNDEBUG
```
