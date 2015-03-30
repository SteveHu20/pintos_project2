#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/page.h"

/*
 * Initialize the frame table, This frame table 
 * in essence is a list, so we need to initialize this list
 * also we need a frame_lock to do synchronization.
 */
void
frame_table_init (void)
{
   list_init (&frame_table);
   lock_init (&frame_table_lock);
}

/*
 * allocate a frame from user pool
 */
void *
frame_alloc (enum palloc_flags flags, struct sup_page_entry *spte)
{
     //if trying to get page from kernel pool, return null
    if ( (flags & PAL_USER) == 0)
     {
 	return NULL;	
     }
     
     void *frame = palloc_get_page (flags);
     //if it's valid frame, add it to frame table, which are used to 
     //keep track all the frame activity
     if (frame)
       {
	   frame_add_to_table (frame, spte);
       }
     else
	{
		while(!frame)
		{
		  frame = frame_evict (flags);
		  lock_release (&frame_table_lock);
		}
		if (!frame)
		{
		   PANIC("Frame could not be evicted because swap is full!");
		}
	  frame_add_to_table (frame,spte);
	}
	return frame;
}
/*
 *  Loop the frame_table, find the freeing frame if any, remove it from
 *  the table list, free the frame table entry it occupies, and most importantly
 *  free all the data inside this frame.
 */
void
frame_free (void *frame)
{
   struct list_elem *e;
   lock_acquire (&frame_table_lock);
   for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next(e))
    {
       struct frame_entry *fte = list_entry  (e, struct frame_entry, elem);
 	if (fte->frame == frame)
	{
		list_remove (e);
		free (fte);
		palloc_free_page (frame);
		break;
	}
    }
   lock_release (&frame_table_lock);
}

/*
 *  Add frame_entry to frame table. Note we have list struct
 *  which are easily causing race condition
 */
void
frame_add_to_table (void *frame, struct sup_page_entry *spte)
{
    struct frame_entry *fte = malloc (sizeof (struct frame_entry));
    fte->frame = frame;
    fte->spte = spte;
    fte->thread = thread_current ();
    lock_acquire (&frame_table_lock);
    list_push_back (&frame_table, &fte->elem);
    lock_release (&frame_table_lock);
} 

void *
frame_evict (enum palloc_flags flags)
{

}
