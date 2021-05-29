#include <stdio.h>
#include <sys/ioctl.h>

// https://blog.csdn.net/qq_19923217/article/details/82698787?utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-1.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-1.control
// https://man7.org/linux/man-pages/man2/ioctl.2.html

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    printf("get window size by ioctl failed\n");

    return -1;
  }

  *cols = ws.ws_col;
  *rows = ws.ws_row;

  return 0;
}


int main(int argc, char const *argv[])
{
  int rows, cols;
  if (getWindowSize(&rows, &cols) == -1) {
    printf("query terminal size failed\n");
    return -1;
  } else {
    printf("terminal size is %dx%d (cols / rows)\n", cols, rows);
    return 0;
  }
}