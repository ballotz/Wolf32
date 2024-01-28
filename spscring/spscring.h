#pragma once

typedef struct
{
    void* buffer;
    int size;
    int rindex;
    int windex;
}
spscring_t;

void spscring_init(spscring_t* r, void* buffer, int size);

int spscring_read(spscring_t* r, void* data, int size);

int spscring_write(spscring_t* r, void* data, int size);

void spscring_clear(spscring_t* r);

int spscring_avail(spscring_t* r);
