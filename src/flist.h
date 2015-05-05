#include <pthread.h>
#include <stdint.h>

// opened file list
struct flist
{
	char        *name;
	char        *real_name;
	int         flags;
	int         fh;
	union
	{
		uint64_t    id;
		struct flist *aid;
	};
	struct flist *next, *prev;
};


// init list system
void flist_init(void);

// create item and rdlock
struct flist* flist_create(const char *name,
	const char *real_name, int flags, int fh);


// return item by id and rdlock
struct flist * flist_item_by_id(uint64_t id);

// return item by id and wrlock
struct flist * flist_item_by_id_wrlock(uint64_t id);

// return all items by item
struct flist ** flist_items_by_eq_name(struct flist * info);

// delete item from locked list
void flist_delete_locked(struct flist * item);

// delete item from wrlocked list
void flist_delete_wrlocked(struct flist * item);

// lock for read
void flist_rdlock(void);
// lock for write
void flist_wrlock(void);
// unlock
void flist_unlock(void);

// wrlock
void flist_wrlock_locked(void);
