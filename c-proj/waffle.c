#include "waffle.h"

int main(int argc, char *argv[]) {
  pthread_t progressThread, convertThread;

  printHeader();

  if (argc == 1) {
    printf("Type 'waffle help' for available commands.\n");
    return(0);
  }

  // Resolve given command and process it.
  if (strncmp(argv[1], CMD_HELP, sizeof(&CMD_HELP)) == 0) {
    printHelp();
    return(0);
  } else if (strncmp(argv[1], CMD_TYPE_CIRCLE, sizeof(&CMD_TYPE_CIRCLE)) == 0) {
    char *inputFile = getInputFilename(argv, argc);

    if (inputFile == NULL) {
      printf("ERROR! No input file given!\n");
      return(1);
    }

    pthread_create(&progressThread, NULL, showProgress, NULL);
    pthread_create(&convertThread, NULL, cropToCircle, inputFile);
    pthread_join(convertThread, NULL);
    pthread_cancel(progressThread);
    return 0;
  } else {
    printf("Command not found! Type 'waffle help' for available commands.\n");
    return(1);
  }

  return(0);
}

// Function returns input filename from arguments.
// Returns NULL if none given.
char *getInputFilename(char *argv[], int argc) {
  char *filename = NULL;
  int i;
  for (i = 2; i < argc; i++) {
    if (strncmp(argv[i], CMD_INPUT_FILE, sizeof(&CMD_INPUT_FILE)) == 0
      || strncmp(argv[i], CMD_INPUT_FILE_SHORT, sizeof(&CMD_INPUT_FILE_SHORT)) == 0) {
        if (i + 1 <= argc) {
          filename = argv[i + 1];
        }
        break;
    }
  }
  return filename;
}

// Function processess given SVG file.
// Determines height and width; creates circle clipPath and applies it.
void *cropToCircle(void *arg) {
  char *input = (char*)arg;
  int height = 0, width = 0;
  printf("Cropping %s to %s...\n", input, CMD_TYPE_CIRCLE);

  // Prepare command for height and width extract
  char *heightCmd = (char*)malloc((sizeof(&input) + 90) * sizeof(char));
  char *widthCmd = (char*)malloc((sizeof(&input) + 90) * sizeof(char));
  sprintf(heightCmd, "grep -m 1 \"<svg *\" %s | grep -o -E \
   'height=\"[0-9]*' | cut -d '\"' -f 2", input);
  sprintf(widthCmd, "grep -m 1 \"<svg *\" %s | grep -o -E \
   'width=\"[0-9]*' | cut -d '\"' -f 2", input);

  FILE *heightOut = popen(heightCmd, "r");
  FILE *widthOut = popen(widthCmd, "r");
  char out[16];
  if (heightOut == NULL || widthOut == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  while (fgets(out, sizeof(out)-1, heightOut) != NULL) {
    height = atoi(out);
  }
  while (fgets(out, sizeof(out)-1, widthOut) != NULL) {
    width = atoi(out);
  }
  free(heightCmd);
  free(widthCmd);

  if (height == 0) {
    printf("ERROR! Svg tag is missing 'height' parameter.\n");
    pthread_exit(NULL);
  }
  if (width == 0) {
    printf("ERROR! Svg tag is missing 'width' parameter.\n");
    pthread_exit(NULL);
  }

  int r = height / 2;
  if (height > width) {
    r = width / 2;
  }

  // Prepare clipPath statements
  char *clipPath = "<clipPath id=\"circle-clip\">\n    <circle cx=\"%d\" cy=\"%d\" r=\"%d\" />\n</clipPath>\n";
  char *outFileName = (char*)malloc((sizeof(&input) + 15) * sizeof(char));
  sprintf(outFileName, "%s_cropped.svg", input);

  FILE *outputFile = fopen(outFileName, "w+");
  FILE *inputFile = fopen(input, "r");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  if (!outputFile || !inputFile) {
    printf("ERROR! Can't create output file.");
    pthread_exit(NULL);
  }

  regex_t regexSvg;
  int retiSvg;
  regex_t regexG;
  int retiG;

  // Prepare regular expression
  retiSvg = regcomp(&regexSvg, "^<svg.*>", 0);
  if (retiSvg) {
      fprintf(stderr, "Could not compile regex\n");
      pthread_exit(NULL);
  }
  retiG = regcomp(&regexG, "<g.*>", 0);
  if (retiG) {
      fprintf(stderr, "Could not compile regex\n");
      pthread_exit(NULL);
  }

  while ((read = getline(&line, &len, inputFile)) != -1) {
    retiSvg = regexec(&regexSvg, line, 0, NULL, 0);
    if (!retiSvg) {
      fprintf(outputFile, line, NULL);
      fprintf(outputFile, clipPath, width / 2, height / 2, r);
      continue;
    }
    retiG = regexec(&regexG, line, 0, NULL, 0);
    if (!retiG) {
      char g[read - 2];
      strncpy(g, line, read - 2);
      g[read - 2] = '\0';
      fprintf(outputFile, "%s clip-path=\"url(#circle-clip)\">\n", g);
      continue;
    }
    fprintf(outputFile, line, NULL);
  }

  printf("DONE!\n");
  pthread_exit(NULL);
}

void *showProgress(void *arg) {
  while (1) {
    advance_cursor();
    usleep(100000);
  }
}

void advance_cursor() {
  static int pos=0;
  char cursor[4]={'/','-','\\','|'};
  printf("%c\b", cursor[pos]);
  fflush(stdout);
  pos = (pos+1) % 4;
}

void printHelp() {
  printf("Usage example:\n");
  printf("  waffle circle -i your.svg -o your_cropped.svg\n");
  printf("\nAvailable cropping types:\n");
  printf("  circle\n");
  printf("\nRequired arguments:\n");
  printf("	-i              Path to input file\n");
  printf("	--input-file\n");
  printf("Optional arguments:\n");
  printf("	-o              Output file path (default: ./*_cropped.svg)\n");
  printf("	--output-file\n");
}

void printHeader() {
  printf("# Waffle. SVG image crop.\n");
  printf("# https://github.com/MSiliunas/waffle\n");
  printf("# Licensed under GNU GPLv3 license\n\n");
}
