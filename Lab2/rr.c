#include <fcntl.h>
#include <stdbool.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <unistd.h>

/* A process table entry.  */
struct process {
  long pid;
  long arrival_time;
  long burst_time;

  TAILQ_ENTRY (process) pointers;

  /* Additional fields here */
  bool running;
  long waiting_time;
  long start_exec_time;
  long remaining_time;
  long response_time;
  long end_time;
  /* End of "Additional fields here" */
};

TAILQ_HEAD (process_list, process);

/* Skip past initial nondigits in *DATA, then scan an unsigned decimal
   integer and return its value.  Do not scan past DATA_END.  Return
   the integer’s value.  Report an error and exit if no integer is
   found, or if the integer overflows.  */
static long next_int (char const **data, char const *data_end) {
  long current = 0;
  bool int_start = false;
  char const *d;

  for (d = *data; d < data_end; d++) {
      char c = *d;
      if ('0' <= c && c <= '9') {
        int_start = true;
        if (ckd_mul (&current, current, 10) || ckd_add (&current, current, c - '0')) {
          fprintf (stderr, "integer overflow\n");
          exit (1);
        }
	    } else if (int_start)
	      break;
  }

  if (!int_start) {
    fprintf (stderr, "missing integer\n");
    exit (1);
  }

  *data = d;
  return current;
}

/* Return the first unsigned decimal integer scanned from DATA.
   Report an error and exit if no integer is found, or if it overflows.  */
static long next_int_from_c_str (char const *data) {
  return next_int (&data, strchr (data, 0));
}

/* A vector of processes of length NPROCESSES; the vector consists of
   PROCESS[0], ..., PROCESS[NPROCESSES - 1].  */
struct process_set {
  long nprocesses;
  struct process *process;
};

/* Return a vector of processes scanned from the file named FILENAME.
   Report an error and exit on failure.  */
static struct process_set init_processes (char const *filename) {
  int fd = open (filename, O_RDONLY);
  if (fd < 0) {
    perror ("open");
    exit (1);
  }

  struct stat st;
  if (fstat (fd, &st) < 0) {
    perror ("stat");
    exit (1);
  }

  size_t size;
  if (ckd_add (&size, st.st_size, 0)) {
    fprintf (stderr, "%s: file size out of range\n", filename);
    exit (1);
  }

  char *data_start = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED) {
    perror ("mmap");
    exit (1);
  }

  char const *data_end = data_start + size;
  char const *data = data_start;

  long nprocesses = next_int (&data, data_end);
  if (nprocesses <= 0) {
    fprintf (stderr, "no processes\n");
    exit (1);
  }

  struct process *process = calloc (sizeof *process, nprocesses);
  if (!process) {
    perror ("calloc");
    exit (1);
  }

  for (long i = 0; i < nprocesses; i++) {
    process[i].pid = next_int (&data, data_end);
    process[i].arrival_time = next_int (&data, data_end);
    process[i].burst_time = next_int (&data, data_end);
    if (process[i].burst_time == 0) {
	    fprintf (stderr, "process %ld has zero burst time\n", process[i].pid);
	    exit (1);
	  }
  }

  if (munmap (data_start, size) < 0) {
    perror ("munmap");
    exit (1);
  }

  if (close (fd) < 0) {
    perror ("close");
    exit (1);
  }

  return (struct process_set) {nprocesses, process};
}

struct IndexedLong {
    long value;
    long index;
};

// Comparison function for qsort
int compareIndexedLongs(const void *a, const void *b) {
    const struct IndexedLong *indexedA = (const struct IndexedLong *)a;
    const struct IndexedLong *indexedB = (const struct IndexedLong *)b;

    if (indexedA->value < indexedB->value) 
      return -1;
    if (indexedA->value > indexedB->value) 
      return 1;
    return indexedA->index - indexedB->index;  // Ensure stability by comparing original indices
}

long calcQuantLength(struct process_list *list, long proc_count) {
    int arr[proc_count];
    int i = 0;

    struct process *temp;
    TAILQ_FOREACH(temp, list, pointers) {
        arr[i++] = temp->burst_time - temp->remaining_time;
    }

    qsort(arr, proc_count, sizeof(int), compareIndexedLongs);

    long median;
    if (proc_count % 2 == 1) {
              median = arr[proc_count / 2];

    } else {
        median = (arr[proc_count / 2 - 1] + arr[proc_count / 2]) / 2;
    }

    if (median == 0) {
      return 1;
    } else {
      return median;
}

}

int main (int argc, char *argv[]) {
  if (argc != 3) {
    fprintf (stderr, "%s: usage: %s file quantum\n", argv[0], argv[0]);
    return 1;
  }

  struct process_set ps = init_processes (argv[1]);
  long quantum_length = (strcmp (argv[2], "median") == 0 ? -1 : next_int_from_c_str (argv[2]));
  if (quantum_length == 0) {
    fprintf (stderr, "%s: zero quantum length\n", argv[0]);
    return 1;
  }

  struct process_list list;
  TAILQ_INIT (&list);

  long total_wait_time = 0;
  long total_response_time = 0;

  /* Your code here */
  long time = 0;
  struct process *p;
  struct process *curr;
  long num_changes = 0;
  long proc_count = 0;
  long quantum_count = 0;
  bool fixed_quantum_length = true;
  bool context_switch = false;

  if (strcmp (argv[2], "median") == 0) {
    fixed_quantum_length = false;
  }
  // Puts processes that arrive first in
  while (TAILQ_EMPTY(&list)) {
    for (long i = 0; i < ps.nprocesses; i++) {
      p = &ps.process[i];
      if (p->arrival_time == time) {
        p->remaining_time = p->burst_time;
        p->running = false;
        p->waiting_time = 0;
        proc_count++;
        TAILQ_INSERT_TAIL(&list, &ps.process[i], pointers);
      }
    }
    time++;
  }
  // iterate through rest of processes
  while (num_changes != ps.nprocesses) {
    curr = TAILQ_FIRST(&list);
    // when process switch
    if (context_switch) {
        for (long i = 0; i < ps.nprocesses; i++) {
            p = &ps.process[i];
            if (p->arrival_time == time) {
                p->remaining_time = p->burst_time;
                p->waiting_time = 0;
                p->running = false;
                proc_count++;
                TAILQ_INSERT_TAIL(&list, &ps.process[i], pointers);
            }
        }
        context_switch = false;
        quantum_count = 0;
        time++;
        continue;
    }
    // if we need quantum length, calc
    if (quantum_length == -1) {
      quantum_length = calcQuantLength(&list, proc_count);
    }

    if (curr->running == false) {
      curr->start_exec_time = time - 1;
      curr->running = true;
    }
    curr->remaining_time--;
    quantum_count++;
    // reinsert process if needed
    if (quantum_count == quantum_length) {
      TAILQ_REMOVE(&list, curr, pointers);
      TAILQ_INSERT_TAIL(&list, curr, pointers);

      // if more than one process count switch
      if (proc_count > 1)
        context_switch = true;
      quantum_count = 0;
      if (!fixed_quantum_length)
        quantum_length = -1;
    }

    for (long i = 0; i < ps.nprocesses; i++) {
      p = &ps.process[i];
      if (p->arrival_time == time) {
        p->running = false;
        p->remaining_time = p->burst_time;
        p->waiting_time = 0;
        proc_count++;
        TAILQ_INSERT_TAIL(&list, &ps.process[i], pointers);
      }
    }

    //if curr's remaining time == 0
    if (curr->remaining_time == 0) {
      curr->end_time = time;
      quantum_count = 0;

      if (!fixed_quantum_length) 
        quantum_length = -1;

      total_wait_time += (curr->end_time - curr->arrival_time - curr->burst_time);
      total_response_time += curr->start_exec_time - curr->arrival_time;
      num_changes++;
      proc_count--;
      TAILQ_REMOVE(&list, curr, pointers);
      // context switch if there are any processes left
      if (proc_count >= 1)
        context_switch = true;
    }
    time++;
  }
  /* End of "Your code here" */

  printf ("Average wait time: %.2f\n",
	  total_wait_time / (double) ps.nprocesses);
  printf ("Average response time: %.2f\n",
	  total_response_time / (double) ps.nprocesses);

  if (fflush (stdout) < 0 || ferror (stdout))
    {
      perror ("stdout");
      return 1;
    }

  free (ps.process);
  return 0;
}
