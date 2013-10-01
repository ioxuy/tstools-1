/* vim: set tabstop=8 shiftwidth=8:
 * name: buddy.h
 * funx: buddy memory pool, to avoid malloc and free from OS frequently
 *
 * tree vs pool:
 *
 *         7          6             0
 *       6   6      0   6         0   0
 *      5 5 5 5    5 5 5 5       5 5 5 5
 *
 *       init     (2^6)-byte   2x(2^6)-byte
 *      status     allocted      allocted
 *
 * 2013-03-09, ZHOU Cheng, modularized
 * 2012-11-02, manuscola.bean@gmail.com, optimized from https://github.com/wuwenbin/buddy2
 */

#ifndef BUDDY_H
#define BUDDY_H

#ifdef __cplusplus
extern "C" {
#endif

#define BUDDY_ORDER_MAX (int)(8 * sizeof(size_t))

/*@null@*/ /*@only@*/ void *buddy_create(int order_max, int order_min);
int buddy_destroy(/*@null@*/ /*@only@*/ void *id);
int buddy_init(void *id);
int buddy_status(void *id, int enable, const char *hint); /* for debug */

/*@null@*/ /*@dependent@*/ void *buddy_malloc(void *id, size_t size);
/*@null@*/ /*@dependent@*/ void *buddy_realloc(void *id, void *ptr, size_t size); /* FIXME: need to be test */
void buddy_free(/*@null@*/ void *id, /*@null@*/ /*@dependent@*/ void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* BUDDY_H */
