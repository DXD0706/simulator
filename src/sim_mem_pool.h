#ifndef _MEMPOOL_H__
#define _MEMPOOL_H__

#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "sim_para.h"

#define MAXBLOCKNUM (20000*48)
#define DATALEN     (256)

typedef struct packData { //node
    struct timeval      tv;
    int                 linkno;
    unsigned short      datalen;
    unsigned char       direction;
    unsigned char       data[DATALEN];
} packData;


typedef struct mem_block_t { //node
    void                *mem_pool;
    mem_block_t         *next;
    struct timeval      tv;
    int                 linkno;
    unsigned short      datalen;
    unsigned short      offset;
    unsigned char       direction;
    unsigned char       data[DATALEN];
} mem_block_t;



/* shared memory */
typedef struct mem_pool_t {
    mem_block_t     *freelist;
    pthread_mutex_t mutex;
    mem_block_t     data[MAXBLOCKNUM];
} mem_pool_t;



extern mem_pool_t   *mem_pool_create(char *poll_name = NULL);
extern mem_block_t  *mem_pool_alloc(mem_pool_t *pool);

extern int mem_pool_destroy(mem_pool_t *pool);
extern int mem_pool_free(mem_pool_t *pool, mem_block_t* mem_block);


extern int link_sdata_insert(struct link_info_t *p_link, mem_block_t *mem_block);
extern int link_sdata_insert(struct link_info_t *p_link, packData *pack, mem_pool_t *pool);
extern int link_sdata_insert(link_info_t *p_link, unsigned char *buf,int len, mem_pool_t *pool);
extern mem_pool_t *g_send_pool;

#endif

