/*
 * Streaming JPEG generator.
 *
 * Copyright 2013-2014 Mycelium SA, Luxembourg.
 *
 * This file is part of Mycelium Entropy.
 *
 * Mycelium Entropy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.  See file GPL in the source code
 * distribution or <http://www.gnu.org/licenses/>.
 *
 * Mycelium Entropy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef JPEG_H_INCLUDED
#define JPEG_H_INCLUDED

#include "layout.h"

//size_t make_jpeg(uint8_t *buf, const struct Layout *layout);

void jpeg_init(uint8_t *buf, uint8_t *end, const struct Layout *l);
uint8_t * jpeg_get_block(unsigned blk);

#endif
