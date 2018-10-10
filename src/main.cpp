#include "../include/encoder.h"
#include "../include/decoder.h"

int main(int argc, char** argv) {

    unsigned char quality = atoi(argv[2]);
    const char *outputFile = argv[3];
    char *inputFile = argv[1];

    RGBImage image = decode(inputFile);

    encodeImage(image, quality, outputFile);
    return 0;
}
