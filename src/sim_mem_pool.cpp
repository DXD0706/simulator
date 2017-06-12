#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include "sim_mem_pool.h"
//#include "sim_ringbuffer.h"

extern int g_epfd_send;

mem_pool_t *mem_pool_create(char *poll_name)
{
    mem_pool_t *pool = NULL;
    pool = (mem_pool_t *)malloc(sizeof(mem_pool_t));
    if (pool == NULL) {
        return NULL;
    }
    memset(pool, 0x00, sizeof(mem_pool_t));
    pthread_mutex_init(&pool->mutex, NULL);
    for (int i = 0; i < ( MAXBLOCKNUM - 1); i++) {
        pool->data[i].next = &pool->data[i + 1];
    }
    pool->freelist = &pool->data[0];
    return pool;
}
int mem_pool_destroy(mem_pool_t *pool)
{
    if (pool == NULL) {
        errno = EINVAL;
        return -1;
    }
    free(pool);
    return 0;
}
mem_block_t* mem_pool_alloc(mem_pool_t *pool)
{
    if (pool == NULL) {
        errno = EINVAL;
        return NULL;
    }
    mem_block_t *mblock = NULL;
    pthread_mutex_lock(&pool->mutex);
    if (pool->freelist != NULL) {
        mblock = pool->freelist;
        pool->freelist = mblock->next;
        mblock->mem_pool = (void*)pool;
        mblock->linkno = 0;
        mblock->tv.tv_sec = 0L;
        mblock->tv.tv_usec = 0L;
        mblock->offset = 0;
        mblock->next = NULL;
        mblock->datalen = 0;
        mblock->direction = 0;
        memset(mblock->data, 0x00, sizeof(mblock->data));
    }
    pthread_mutex_unlock(&pool->mutex);
    return mblock;
}
int mem_pool_free(mem_pool_t *pool, mem_block_t *mem_block)
{
    if (pool == NULL || mem_block == NULL) {
        errno = EINVAL;
        return -1;
    }
    pthread_mutex_lock(&pool->mutex);
    mem_block->next = pool->freelist;
    pool->freelist = mem_block;
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

int mem_pool_free( mem_block_t *mem_block)
{
    if (mem_block == NULL) {
        errno = EINVAL;
        return -1;
    }

    mem_pool_t *pool = (mem_pool_t*)mem_block->mem_pool;
    if (pool == NULL) {
        return -1;
    }

    pthread_mutex_lock(&pool->mutex);
    mem_block->next = pool->freelist;
    pool->freelist = mem_block;
    pthread_mutex_unlock(&pool->mutex);

    return 0;
}

int link_sdata_insert(link_info_t *p_link, mem_block_t *mem_block)
{
    if (p_link == NULL || mem_block == NULL) {
        errno = EINVAL;
        return -1;
    }

    mem_block->next = NULL;
    mem_block->offset = 0;
    pthread_mutex_lock(&p_link->send_list_mutex);
    if (p_link->send_list == NULL) {
        p_link->send_list = mem_block;
    } else {
        mem_block_t *p = p_link->send_list;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = mem_block;
    }
    pthread_mutex_unlock(&p_link->send_list_mutex);
    return 0;
}

int link_sdata_insert(link_info_t *p_link, packData *pack, mem_pool_t *pool)
{
    if (p_link == NULL || pack == NULL) {
        errno = EINVAL;
        return -1;
    }
    mem_block_t *mem_block = mem_pool_alloc(pool);
    if (mem_block == NULL) {
        return -1;
    }
    memset(mem_block, 0x00, sizeof(mem_block_t));
    mem_block->linkno = pack->linkno;
    mem_block->datalen = pack->datalen;
    mem_block->tv = pack->tv;
    mem_block->direction = 1;
    mem_block->offset = 0;
    memcpy(&mem_block->data, pack->data, pack->datalen);

    return link_sdata_insert(p_link, mem_block);
}

int link_sdata_insert(link_info_t *p_link, unsigned char *buf, int len, mem_pool_t *pool)
{
    if (p_link == NULL || buf == NULL || len <= 0) {
        errno = EINVAL;
        return -1;
    }
    mem_block_t *mem_block = mem_pool_alloc(pool);
    if (mem_block == NULL) {
        printf("mem_pool_alloc failed\n");
        fflush(stdout);
        exit(1);
        return -1;
    }
    mem_block->linkno = p_link->linkno;
    mem_block->datalen = len;
    /*if (mem_block->offset != 0) {
        printf("offset error %d\n",mem_block->offset);
        //exit(1);
    }*/
    mem_block->offset = 0;
    gettimeofday(&mem_block->tv, NULL);
    mem_block->direction = 1;
    memcpy(&mem_block->data, buf, len);

    return link_sdata_insert(p_link, mem_block);
}


