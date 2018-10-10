#ifndef DECODER_H
#define DECODER_H

#include <cstdio>
#include <iostream>
#include <cstring>
#include "tables.h"

RGBImage decode(char *file);

int decodePPM2RGB(char *file, RGBImage &img);

#endif // DECODER_H
