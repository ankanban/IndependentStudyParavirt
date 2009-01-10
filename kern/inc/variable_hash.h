#ifndef __VARIABLE_HASH_H__

#define __VARIABLE_HASH_H__

#include <variable_queue.h>


#define HASH_NEW(H_TYPE, H_LIST_TYPE, H_KEY_TYPE, H_VAL_TYPE, H_SIZE)	\
  typedef struct {							\
    H_LIST_TYPE hash[H_SIZE];						\
    uint32_t (*hashfn)(H_KEY_TYPE key);					\
    int (*hashcmp)(H_KEY_TYPE k1, H_KEY_TYPE k2);			\
    H_KEY_TYPE (*hashkey)(H_VAL_TYPE * elem);				\
    int size;								\
  } H_TYPE


#define HASH_INIT(H_TABLE, H_SIZE, H_HASHFN, H_HASHCMP, H_HASHKEY) \
  do {								   \
    uint32_t i = 0;							   \
    for (; i < H_SIZE; i++) {					   \
      Q_INIT_HEAD(&((H_TABLE)->hash[i]));			   \
    }								   \
    (H_TABLE)->size = H_SIZE;					   \
    (H_TABLE)->hashfn = H_HASHFN;				   \
    (H_TABLE)->hashcmp = H_HASHCMP;				   \
    (H_TABLE)->hashkey = H_HASHKEY;				   \
  } while(0)

#define HASH_INSERT(H_TABLE, H_KEY, H_VAL, H_LINK)			\
  do {									\
    uint32_t index = (H_TABLE)->hashfn((H_KEY));			\
    Q_INSERT_TAIL(&((H_TABLE)->hash[index]),				\
		  (H_VAL), H_LINK);					\
  } while(0)

#define HASH_REMOVE(H_TABLE, H_KEY, H_VAL, H_LINK)		\
  do {								\
    uint32_t index = (H_TABLE)->hashfn((H_KEY));		\
    Q_REMOVE(&((H_TABLE)->hash[index]), (H_VAL), H_LINK);	\
  } while(0)

#define HASH_LOOKUP(H_TABLE, H_KEY, H_VAL_PTR, H_LINK)			\
  do {									\
    *(H_VAL_PTR) = NULL;						\
    uint32_t index = (H_TABLE)->hashfn(H_KEY);				\
    Q_FOREACH((*(H_VAL_PTR)),	                                        \
	      &((H_TABLE)->hash[index]), H_LINK) {			\
      if ((H_TABLE)->hashcmp((H_KEY),					\
			     (H_TABLE)->hashkey((*(H_VAL_PTR)))) == 0) { \
	break;								\
      }									\
    }									\
  } while(0)

#endif /* __VARIABLE_HASH_H__ */
