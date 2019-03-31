#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ocradlib.h>
#include <allheaders.h>
#include <capi.h>

char *read_file(FILE *f) {
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *str = (char*) malloc(size + 1);

    fread(str, size, 1, f);
    fclose(f);

    str[size] = '\0';

    return str;
}

int check_text(char *text, char *correct_text, bool is_ocrad) {
    int n = 0;
    int j = 0;

    if (strlen(text) < strlen(correct_text)) {
        printf("WARN: Supplied text is shorter than the correct text! text: %d correct_text: %d\n", strlen(text), strlen(correct_text));
        
        for (int i = 0; i < strlen(text); i++) {
            // printf("CORRECT: %c SUPPLIED: %c GOOD? %s\n", correct_text[j], text[i], (text[i] == correct_text[j] ? "YES" : "NO"));
            if (text[i] == correct_text[j])
                n++;

            j++;

            if (text[i] == '\n' && is_ocrad)
                j--;
        }
    } else if (strlen(text) > strlen(correct_text)) {
        printf("WARN: Supplied text is longer than the correct text! text: %d correct_text: %d\n", strlen(text), strlen(correct_text));

        for (int i = 0; i < strlen(correct_text); i++) {
            // printf("CORRECT: %c SUPPLIED: %c GOOD? %s\n", correct_text[j], text[i], (text[i] == correct_text[j] ? "YES" : "NO"));
            if (text[i] == correct_text[j])
                n++;

            j++;

            if (text[i] == '\n' && is_ocrad)
                j--;
        }
    } else if (strlen(text) == strlen(correct_text)) {
        for (int i = 0; i < strlen(correct_text); i++) {
            // printf("CORRECT: %c SUPPLIED: %c GOOD? %s\n", correct_text[j], text[i], (text[i] == correct_text[j] ? "YES" : "NO"));
            if (text[i] == correct_text[j])
                n++;

            j++;

            if (text[i] == '\n' && is_ocrad)
                j--;
        }
    }

    return n;
}

void show_ocrad_info(struct OCRAD_Descriptor *ocrad, char *correct_text) {
    int blocks = OCRAD_result_blocks(ocrad);
    printf("Text blocks in image: %d\n", blocks);

    for (int i = 0; i < blocks; i++) {
        int lines = OCRAD_result_lines(ocrad, i);
        char *text = NULL;
        int len = 0;

        printf("Text lines in block number %d: %d\nCharacters in this block: %d\n", i, OCRAD_result_lines(ocrad, i), OCRAD_result_chars_block(ocrad, i));
        for (int j = 0; j < lines; j++) {
            printf("%s", OCRAD_result_line(ocrad, i, j));

            len += strlen(OCRAD_result_line(ocrad, i, j));
        }

        text = (char*) malloc(len + lines);

        text[0] = '\0';

        for (int j = 0; j < lines; j++) {
            strcat(text, OCRAD_result_line(ocrad, i, j));

            if (i < (lines - 1))
                strcat(text, " ");
        }

        int n = check_text(text, correct_text, true);
        int m = strlen(correct_text);
        printf("N: %d M: %d\n", n, m);
        printf("OCRAD results: %d AVERAGE: %f\n", n, (float) ((float) n / (float) m) * 100);

        free(text);
    }
}

int test_ocrad(char *image, char *correct_text) {
    printf("IMG: %s\n", image);
    struct OCRAD_Descriptor *ocrad = OCRAD_open();
    printf("OCRAD_open: errno: %d\n", OCRAD_get_errno(ocrad));
    printf("Init OCRAD: Ver: %s\n", OCRAD_version());

    if (OCRAD_set_image_from_file(ocrad, image, false) < 0) {
        fprintf(stderr, "OCRAD_set_image_from_file: ERR! errno: %d\n", OCRAD_get_errno(ocrad));
        return 1;
    }

    if (OCRAD_recognize(ocrad, true) < 0) {
        fprintf(stderr, "OCRAD_recognize: ERR! errno: %d\n", OCRAD_get_errno(ocrad));
        return 1;
    }

    show_ocrad_info(ocrad, correct_text);

    OCRAD_close(ocrad);

    return 0;
}

int test_tesseract(char *image, char *correct_text) {
    TessBaseAPI *tesseract = NULL;
    PIX *img = NULL;

    char *result = NULL;

    if ((img = pixRead(image)) == NULL) {
        fprintf(stderr, "pixRead: ERR! Failed to read image\n");
        return 1;
    }

    tesseract = TessBaseAPICreate();

    if ((TessBaseAPIInit3(tesseract, NULL, "eng")) != 0) {
        fprintf(stderr, "TessBaseAPIInit3: ERR! Failed to initialize tesseract\n");
        return 1;
    }

    TessBaseAPISetImage2(tesseract, img);

    if(TessBaseAPIRecognize(tesseract, NULL) != 0) {
        fprintf(stderr, "TessBaseAPIRecognize: ERR! Error in recognition\n");
        return 1;
    }

    if ((result = TessBaseAPIGetUTF8Text(tesseract)) == NULL) {
        fprintf(stderr, "TessBaseAPIGetUTF8Text: ERR! Error during text retrieval\n");
        return 1;
    }

    printf("Tesseract result:\n%s\n", result);

    int n = check_text(result, correct_text, false);
    int m = strlen(result);
    printf("N: %d M: %d\n", n, m);
    printf("Tesseract results: %d AVERAGE: %f\n", n, (float) ((float) n / (float) m) * 100);

    TessDeleteText(result);
    TessBaseAPIEnd(tesseract);
    TessBaseAPIDelete(tesseract);
    pixDestroy(&img);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Missing arguments\n");
        return 1;
    }

    FILE *f = fopen(argv[2], "r");
    char *correct_text = read_file(f);
    int exit = test_ocrad(argv[1], correct_text) + test_tesseract(argv[1], correct_text);

    free(correct_text);

    return exit == 0 ? 0 : 1;
}
