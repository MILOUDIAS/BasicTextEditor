#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

//---------------------------- CURRENTLY I'M in section: # Window size, the hard way

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/
struct editorConfig {

  int screen_rows;
  int screen_cols;

  // reference termios struct
  struct termios original_termios;
};
struct editorConfig E;

/*** terminal ***/

void die(const char *s) {

  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(EXIT_FAILURE);
}

void disableRawMode() {

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1) {
    die("tcsetattr");
  }
}

void enableRawMode() {

  if (tcgetattr(STDIN_FILENO, &E.original_termios) == -1) {

    die("tcgetattr");
  }

  atexit(disableRawMode);
  struct termios raw = E.original_termios;

  tcgetattr(STDIN_FILENO, &raw);
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

char editorReadKey() {
  int nread;
  char c;
  while( ( nread = read( STDIN_FILENO, &c, 1 ) ) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  return c;
}

int getCursorPosition(int *rows, int *cols) {

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  printf("\r\n");
  char c;

  while (read(STDIN_FILENO, &c, 1) == 1) {

    if (iscntrl(c)) {

      printf("%d\r\n", c);

    } else {

      printf("%d ('%c')\r\n", c, c);
    }
  }

  editorReadKey();

  return EXIT_FAILURE;
}

int getWindowSize(int *rows, int *cols) {

  struct winsize ws;
  if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {

    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;

    return getCursorPosition(rows, cols);

  } else {

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 0;
  }
}

/*** input ***/
void editorProcessKeypress() {

  char c = editorReadKey();

  switch (c) {

  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    exit(EXIT_SUCCESS);
    break;
  }
}

/*** output ***/

void editorDrawRows() {

  int y;
  for (y = 0; y < E.screen_rows; y++) {

    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void editorRefreshScreen() {

  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** init ***/

void initEditor() {

  if( getWindowSize(&E.screen_rows, &E.screen_cols) == -1 ) die( "getWindowSize" );

}

int main(int argc, char **argv) {

  enableRawMode();
  initEditor();

  while (1) {

    editorRefreshScreen();
    editorProcessKeypress();
  }

  return EXIT_SUCCESS;
}
