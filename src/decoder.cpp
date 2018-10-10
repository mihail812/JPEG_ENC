#include "../include/decoder.h"



RGBImage decode(char *file)
{
    RGBImage img;
    char* lastDot = strrchr(file, '.');
    int pos = lastDot - file;
    char fmt[3];
    for (int i = pos + 1; file[i] != NULL; i++) {
        fmt[i - pos - 1] = file[i];
    }
    if(strcmp(fmt, "PPM")) {
         int ret = decodePPM2RGB(file, img);
         if (ret)
             printf("Error while decoding of input image:%s", file);
    }
    return img;
}

int decodePPM2RGB(char *file, RGBImage &img)
{
    char line[128];
    char *tok;
    FILE *fp = fopen(file, "rb");
    if(fp == NULL) {
        printf("Error while opening file\n");
        return -1;
    }
    if(fgets(line, 128, fp) == NULL) {
        printf("Error while extracting file");
        fclose(fp);
        return -1;
    }
    if(strcmp(line, "P6\n")) {
        printf("Support only PPM P6");
        fclose(fp);
        return -1;
    }
    while(fgets(line, 128, fp)) {
        if(line[0] == '#')
            continue;
        else {
            tok = strtok(line, " ");
            img.w = atoi(tok);
            tok = strtok(NULL, " ");
            img.h = atoi(tok);
            (void)fgets(line, 128, fp);
            break;
        }
    }

#ifdef __cplusplus
    img.pixel = (rgb*)malloc(img.w * img.h * sizeof(rgb));
#else
    img.pixel = malloc(img.w * img.h * sizeof(rgb));
#endif
    if(img.pixel == NULL) {
        //fprintf(stderr, "Memory Allocation failed");

        fclose(fp);
        return -1;
    }

    (void)fread(img.pixel, sizeof(rgb), img.w * img.h, fp);



    return 0;
}
