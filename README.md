# softrend
A simple software renderer inspired by @rexim. I won't be working as much in this project for the time being. I wanted to learn a bit of 3D graphics by doing this and I have achieved my goals. If @rexim continues with his project I may get more inspired and try to further this project.

## Quickstart
If you are using X11 do the following:
```shell
cc -O3 -o main main.c -l1X11 -lXrandr -lm
```
Make sure to enable optimization for inlining to take place.

On other Display Managers use have to link to the appropriate libraries. (For Wayland or Windows/MacOS etc)
Make sure you use optimization as we are using some `inline` functions.
