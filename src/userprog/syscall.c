#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
 
 
static int sys_halt (void);
static int sys_exit (int status);
static int sys_exec (const char *ufile);
static int sys_wait (tid_t);
static int sys_create (const char *ufile, unsigned initial_size);
static int sys_remove (const char *ufile);
static int sys_open (const char *ufile);
static int sys_filesize (int handle);
static int sys_read (int handle, void *udst_, unsigned size);
static int sys_write (int handle, void *usrc_, unsigned size);
static void sys_seek (int handle, unsigned position);
static unsigned int sys_tell (int handle);
static int sys_close (int handle);
 
static void syscall_handler (struct intr_frame *);
static void copy_in (void *, const void *, size_t);
 
/* Serializes file system operations. */
static struct lock fs_lock;
 
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&fs_lock);
}
 
/* System call handler. */

static void
syscall_handler (struct intr_frame *f)
{  
	typedef int syscall_function(int, int, int);
	
	struct syscall
	{
		size_t arg_cnt;				//Number of Arguments
		syscall_function *func;		//Implementation
	};
		
		/* Table of system calls */
		static const struct syscall syscall_table[] = 
		{
			{0, (syscall_function *) sys_halt},
			{1, (syscall_function *) sys_exit},	
			{1, (syscall_function *) sys_exec},
			{1, (syscall_function *) sys_wait},
			{2, (syscall_function *) sys_create},
			{1, (syscall_function *) sys_remove},
			{1, (syscall_function *) sys_open},			
			{1, (syscall_function *) sys_filesize},
			{3, (syscall_function *) sys_read},
			{3, (syscall_function *) sys_write},
			{2, (syscall_function *) sys_seek},
			{1, (syscall_function *) sys_tell},
			{1, (syscall_function *) sys_close},
		};
		
	const struct syscall *sc;
	unsigned call_nr;
	int args[3];
	
	
	/* Get the system call. */
	copy_in(&call_nr, f->esp, sizeof call_nr);
	
	if(call_nr >= sizeof syscall_table / sizeof *syscall_table)
		sys_exit(-1);
	sc = syscall_table + call_nr;
	
	ASSERT(sc->arg_cnt <= sizeof args / sizeof *args);
	memset(args, 0, sizeof args);
	copy_in(args, (uint32_t *) f->esp + 1, sizeof *args * sc->arg_cnt);
	
	/* Execute the system call
	 * and set the return value. */
	f->eax = sc->func(args[0], args[1], args[2]);

}

/* Returns true if UADDR is a valid, mapped user address,
   false otherwise. */
static bool
verify_user (const void *uaddr) 
{
  return (uaddr < PHYS_BASE
          && pagedir_get_page (thread_current ()->pagedir, uaddr) != NULL);
}
 
/* Copies a byte from user address USRC to kernel address DST.
   USRC must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static inline bool
get_user (uint8_t *dst, const uint8_t *usrc)
{
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
       : "=m" (*dst), "=&a" (eax) : "m" (*usrc));
  return eax != 0;
}
 
/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static inline bool
put_user (uint8_t *udst, uint8_t byte)
{
  int eax;
  asm ("movl $1f, %%eax; movb %b2, %0; 1:"
       : "=m" (*udst), "=&a" (eax) : "q" (byte));
  return eax != 0;
}
 
/* Copies SIZE bytes from user address USRC to kernel address
   DST.
   Call thread_exit() if any of the user accesses are invalid. */
static void
copy_in (void *dst_, const void *usrc_, size_t size) 
{
  uint8_t *dst = dst_;
  const uint8_t *usrc = usrc_;
	
  for (; size > 0; size--, dst++, usrc++) 
    if (usrc >= (uint8_t *) PHYS_BASE || !get_user (dst, usrc)) 
      sys_exit (-1);
}
 
/* Creates a copy of user string US in kernel memory
   and returns it as a page that must be freed with
   palloc_free_page().
   Truncates the string at PGSIZE bytes in size.
   Call thread_exit() if any of the user accesses are invalid. */
static char *
copy_in_string (const char *us) 
{
  char *ks;
  size_t length;
 
  ks = palloc_get_page (PAL_ZERO | PAL_ASSERT);
  if (ks == NULL)
    sys_exit(-1);
 
  for (length = 0; length < PGSIZE; length++)
    {
      if (us >= (char *) PHYS_BASE || !get_user (ks + length, us++)) 
        {
          palloc_free_page (ks);
          sys_exit(-1); 
        }
      if (ks[length] == '\0')
        return ks;
    }
  ks[PGSIZE - 1] = '\0';
  return ks;
}
 
/* Halt system call. */
static int
sys_halt (void)
{
  shutdown_power_off ();
}
 
/* Exit system call. */
static int
sys_exit (int exit_code) 
{
  thread_current ()->wait_status->exit_code = exit_code;
  thread_exit ();
  NOT_REACHED ();
}
 
/* Exec system call. */
static int
sys_exec (const char *ufile) 
{
	
	if(ufile==NULL || !verify_user(ufile))
	{
		sys_exit(-1);
		NOT_REACHED ();
	}
	char *kfile = copy_in_string(ufile);
	int t_tid = process_execute(kfile);
	palloc_free_page(kfile);
	return  t_tid;
	 
}
 
/* Wait system call. */
static int
sys_wait (tid_t child) 
{
  return process_wait(child);
}
 
/* Create system call. */
static int
sys_create (const char *ufile, unsigned initial_size) 
{
	bool sucess = false;
	if(ufile==NULL || !verify_user(ufile))
	{
		//close the program
		sys_exit(-1);
		NOT_REACHED ();
	}
	char *kfile = copy_in_string(ufile);
	lock_acquire (&fs_lock);
	if (kfile != NULL)
	{
		sucess = filesys_create (kfile, (off_t)initial_size); 
	}
	lock_release (&fs_lock);
	 palloc_free_page(kfile);
  return sucess;
}
 
/* Remove system call. */
static int
sys_remove (const char *ufile) 
{
	if(ufile==NULL || !verify_user(ufile))
	{
		//close the program
		sys_exit(-1);
		NOT_REACHED ();
	}
	bool sucess = false;
	char *kfile = copy_in_string(ufile);
	lock_acquire (&fs_lock);
	if (kfile != NULL)
	{
		sucess = filesys_remove (kfile) ; 
	}
	lock_release (&fs_lock);
	 palloc_free_page(kfile);
  return sucess;
}
 
/* A file descriptor, for binding a file handle to a file. */
struct file_descriptor
  {
    struct list_elem elem;      /* List element. */
    struct file *file;          /* File. */
    int handle;                 /* File handle. */
  };
 
/* Open system call. */
static int
sys_open (const char *ufile) 
{
  if(ufile==NULL || !verify_user(ufile))
	{
		//close the program
		sys_exit(-1);
		NOT_REACHED ();
	}
  char *kfile = copy_in_string (ufile);
  
  struct file_descriptor *fd;
  int handle = -1;
 
  fd = malloc (sizeof *fd);
  if (fd != NULL)
    {
      lock_acquire (&fs_lock);
      fd->file = filesys_open (kfile);
      if (fd->file != NULL)
        {
          struct thread *cur = thread_current ();
          handle = fd->handle = cur->next_handle++;
          list_push_front (&cur->fds, &fd->elem);
        }
      else 
        free (fd);
      lock_release (&fs_lock);
    }
  
  palloc_free_page (kfile);
  return handle;
}
 
/* Returns the file descriptor associated with the given handle.
   Terminates the process if HANDLE is not associated with an
   open file. */
static struct file_descriptor *
lookup_fd (int handle)
{
/* Add code to lookup file descriptor in the current thread's fds */
  
  struct thread* cur = thread_current();
  if (list_empty(&(cur->fds))) sys_exit(-1);
  struct list_elem* e = list_front(&(cur->fds));
  
  struct file_descriptor * e2;
  while(e != NULL){
	  e2 = list_entry(e, struct file_descriptor, elem);
	  if (e2->handle == handle){ 
		  return e2;
	  }
	  e = list_next(e);
  }
  sys_exit(-1);
}
 
/* Filesize system call. */
static int
sys_filesize (int handle) 
{
  struct file_descriptor * e2 = lookup_fd(handle);
  lock_acquire (&fs_lock);
  int len = file_length( e2->file);
  lock_release (&fs_lock);
  return len;
}
 
/* Read system call. */
static int
sys_read (int handle, void *udst_, unsigned size) 
{
	int read;
	if(udst_==NULL || !verify_user(udst_))
	{
		//close the program
		sys_exit(-1);
		NOT_REACHED ();
	}
	struct file_descriptor* fd = lookup_fd(handle); 
	
	if(fd == NULL) sys_exit(-1);
	
	lock_acquire (&fs_lock);
	read = file_read(fd->file, udst_, size);
	lock_release (&fs_lock);
  
	return read;
}
 
/* Write system call. */
static int
sys_write (int handle, void *usrc_, unsigned size) 
{
  if(usrc_==NULL || !verify_user(usrc_))
	{
		//close the program
		sys_exit(-1);
		NOT_REACHED ();
	}
	
  uint8_t *usrc = usrc_;
  struct file_descriptor *fd = NULL;
  int bytes_written = 0;
  
  /* Lookup up file descriptor. */
  if (handle != STDOUT_FILENO)
    fd = lookup_fd (handle);

  lock_acquire (&fs_lock);
  while (size > 0) 
    {
      /* How much bytes to write to this page? */
      size_t page_left = PGSIZE - pg_ofs (usrc);
      size_t write_amt = size < page_left ? size : page_left;
      off_t retval;

      /* Do the write. */
      if (handle == STDOUT_FILENO)
        {
          putbuf (usrc, write_amt);
          retval = write_amt;
        }
      else
        retval = file_write (fd->file, usrc, write_amt);
      if (retval < 0) 
        {
          if (bytes_written == 0)
            bytes_written = -1;
          break;
        }
      bytes_written += retval;

      /* If it was a short write we're done. */
      if (retval != (off_t) write_amt)
        break;

      /* Advance. */
      usrc += retval;
      size -= retval;
    }
  lock_release (&fs_lock);
 
  return bytes_written;
}
 
/* Seek system call. */
static void
sys_seek (int handle, unsigned position) 
{
  struct file_descriptor * fd = lookup_fd (handle);
  if (fd!= NULL){
	  lock_acquire (&fs_lock);
	  file_seek(fd->file, position);
	  lock_release (&fs_lock);
  }
}
 
/* Tell system call. */
static unsigned int
sys_tell (int handle) 
{
  struct file_descriptor * fd = lookup_fd (handle);
  unsigned int next;
  if (fd!= NULL){
	  lock_acquire (&fs_lock);
	  next = file_tell(fd->file);
	  lock_release (&fs_lock);
	  return next;
  }
  return -1;
}
 
/* Close system call. */
static int
sys_close (int handle) 
{
	struct file_descriptor *fd = lookup_fd(handle);
	if (fd != NULL)
	{
		struct file *closeFile = fd->file;
		
		if(closeFile != NULL)
		{
			lock_acquire (&fs_lock);
			file_close (closeFile);
			list_remove(&fd->elem);
			lock_release (&fs_lock);
			return 1;
		}
	}
	return 0;
}
 
/* On thread exit, close all open files. */
void
syscall_exit (void) 
{
  return;
}
