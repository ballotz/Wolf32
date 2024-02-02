#include "spscbuffer.h"
#include <string.h>

void spscbuffer_init(spscbuffer_t* r, void* buffer, int size)
{
    r->buffer = buffer;
    r->size = size;
    r->rindex = 0;
    r->windex = 0;
}

int spscbuffer_read(spscbuffer_t* r, void* data, int size)
{
    char* rbuffer = r->buffer;
    int rsize = r->size;
    int rrindex = r->rindex;
    int rwindex = r->windex;
    int avail, blocksize, blocksize2;

    if (rrindex < rwindex)
    {
        avail = rwindex - rrindex;
        blocksize = avail < size ? avail : size;
        memcpy(data, &rbuffer[rrindex], blocksize);
        r->rindex = (rrindex + blocksize) % rsize;
        return blocksize;
    }

    if (rrindex > rwindex)
    {
        avail = rsize - rrindex;
        blocksize = avail < size ? avail : size;
        memcpy(data, &rbuffer[rrindex], blocksize);
        data = (char*)data + blocksize;
        size -= blocksize;
        blocksize2 = rwindex < size ? rwindex : size;
        memcpy(data, rbuffer, blocksize2);
        blocksize += blocksize2;
        r->rindex = (rrindex + blocksize) % rsize;
        return blocksize;
    }

    return 0;
}

int spscbuffer_write(spscbuffer_t* r, const void* data, int size)
{
    char* rbuffer = r->buffer;
    int rsize = r->size;
    int rrindex = r->rindex;
    int rwindex = r->windex;
    int avail, blocksize, blocksize2;

    if (rwindex < rrindex)
    {
        avail = rrindex - rwindex - 1;
        blocksize = avail < size ? avail : size;
        memcpy(&rbuffer[rwindex], data, blocksize);
        r->windex = (rwindex + blocksize) % rsize;
        return blocksize;
    }
    else
    {
        avail = rsize - rwindex - !rrindex;
        blocksize = avail < size ? avail : size;
        memcpy(&rbuffer[rwindex], data, blocksize);
        data = (char*)data + blocksize;
        size -= blocksize;
        avail = rrindex - !!rrindex;
        blocksize2 = avail < size ? avail : size;
        memcpy(rbuffer, data, blocksize2);
        blocksize += blocksize2;
        r->windex = (rwindex + blocksize) % rsize;
        return blocksize;
    }
}

//void spscbuffer_clear(spscbuffer_t* r)
//{
//    r->rindex = r->windex;
//}

int spscbuffer_avail(spscbuffer_t* r)
{
    int rsize = r->size;
    int rrindex = r->rindex;
    int rwindex = r->windex;
    if (rrindex < rwindex)
        return rwindex - rrindex;
    if (rrindex > rwindex)
        return rsize - rrindex + rwindex;
    return 0;
}
