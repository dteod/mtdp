/* Copyright (C) 2021-2022 Domenico Teodonio

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <unity.h>

#include "mtdp/buffer.h"
#include "impl/buffer.h"
#include <time.h>
#include <sys/time.h>

mtdp_buffer_fifo g_fifo;
mtdp_buffer_fifo* fifo = &g_fifo;

void setUp()
{
    mtdp_buffer_fifo_init(fifo);
}

void tearDown()
{
    mtdp_buffer_fifo_destroy(fifo);
}

void test_black_box()
{
    const size_t PUSH_ATTEMPTS = 100, POP_ATTEMPTS = 120;
    TEST_ASSERT_EQUAL(mtdp_buffer_fifo_size(fifo), 0);

    mtdp_buffer buf = NULL;
    for(size_t i = 0; i != PUSH_ATTEMPTS; ++i) {
        mtdp_buffer_fifo_push_back(fifo, &i);
        TEST_ASSERT_EQUAL(mtdp_buffer_fifo_size(fifo), i + 1);
    }
    
    for(size_t i = 0; i != POP_ATTEMPTS; ++i) {
        if(mtdp_buffer_fifo_pop_front(fifo, &buf)) {
            TEST_ASSERT_EQUAL(mtdp_buffer_fifo_size(fifo), PUSH_ATTEMPTS - i - 1);
            TEST_ASSERT_EQUAL(i, buf);
        } else {
            TEST_ASSERT_GREATER_OR_EQUAL(PUSH_ATTEMPTS, i);
        }
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_black_box);
    UNITY_END();
}