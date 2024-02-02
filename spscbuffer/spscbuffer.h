#pragma once

typedef struct
{
    void* buffer;
    int size;
    int rindex;
    int windex;
}
spscbuffer_t;

void spscbuffer_init(spscbuffer_t* r, void* buffer, int size);

int spscbuffer_read(spscbuffer_t* r, void* data, int size);

int spscbuffer_write(spscbuffer_t* r, const void* data, int size);

//void spscbuffer_clear(spscbuffer_t* r);

int spscbuffer_avail(spscbuffer_t* r);
