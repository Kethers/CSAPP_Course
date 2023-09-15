# Solving Malloc Lab Step by Step

## Clarifications of some Concepts

1. Size in header indicates the total size of the block, not just the payload.
2. Segregated lists use explicit list for each different “size level” list.
3. Alignment is for the payload, which means the header can be not aligned.
4. Explicit list will split big block into small blocks, the remaining non-payload block will be inserted into some other level list.
5. Freeing a block will cause coalescing, by checking the neighbor memory space is freed or not instead of checking the prev or next block in list. Be cautious about that.
6. The prealloc bit is set when manipulating its prev-neighbor block.



## Target

1. Implement the segregated list.
2. Use address-ordered policy when freeing.
3. Use first-fit policy when mallocing.
4. Implement the heap consistency checker for better debugging.



## Combine the Theory and implementation

In the course, we learnt implicit free list first. Then the explicit one and segregated one that takes explicit list as basis. It seems that we don’t care about implicit list anymore. However, recall from the course we heard many times that professor emphasized that memory is actually a byte array. When we implement segregated list, we take blocks from the “bytes array” and “blocks array”. When we split or coalesce, the whole heap becomes the implicit free list since we need to deal with its neighbor blocks. So we can similarly use some tricks introduced in the course like setting the epilogue block with size = 0 and alloc bit = 1.



## When to Coalesce or split?

Coalesce: any time when a new free block is spawned, coalesce should take place. Free a allocated block and split a large block both spawn a new free block.

Split: any time when a block is taken for allocation, split should take place. Find a block already in free list or extend the heap to satisfied the aligned payload size. Then try to split that block and inserted the split “small” block into the free list.

When coalesce and split take place, we need to manipulate the free list. Find the right-level free list and insert the block to it, before which remove block from the old-level free list if it was in some free list.



## emo: nasty and tricky beast

As professor said in class, **“Dynamic memory management is very nasty since the bug causes fault can be very far from where you write the bug”**. And it’s emphasized in lab tutorial pdf again **“Dynamic memory allocators are notoriously tricky beasts”**. During the development and debugging of this lab, I think I have a damn deeper understanding of them. It’s really hard to find and even harder to identify the bug. That’s why the recitation slides and lab tutorial all suggest student to start early because this lab indeed takes a lot of time.



## Missing Traces

The handout tar file doesn’t have trace files for testing so we have to download them from somewhere. Copy them under your working dir and change the macro TRACEDIR in `config.h` to `“./traces/”`.



## How to Debug

Use vscode + ssh + gdb to debug. To make it work we need to set `launch.json` and `tasks.json` right.

A simple way is to modify the makefile. Change CFLAGS as follow:

```makefile
# old, for final testing
CFLAGS = -Wall -O2 -m32

# for debugging
CFLAGS = -Wall -Og -m32 -g
```

We can also use cmd like this to output them in a txt file for better debugging:

```shell
$ ./mdriver -f traces/random-bal.rep > debug.txt
```



### Heap Consistency checker

In tutorial, it’s suggested to use macro to warp the `mm_check()` function. It might be helpful but not enough. We have better done it by checking that at some extremely bad cases such as segmentation fault or ran out of heap memory. What’s more, add below code at the end of the loop of `eval_mm_valid()`  can also be helpful (although the program also indicates line of track file when overlapping blocks or some other cases, it won’t output that when seg fault. At this time, below code will help).

```c
printf("%d %d %d\n", trace->ops[i].type, trace->ops[i].index, trace->ops[i].size);
fflush(stdout);
```



## Trick of Splitting

ref: [高性能 Malloc Lab —— 不上树 97/100 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/374478609)

In this blog it says that when required blocks is bigger than 64B, we can place it at the end of the free block instead at the front of the free block, before splitting. But it seems not working for increasing the utility points when testing 2 realloc traces.



## Note

My segregate free list implementation use ***payload size*** instead of block size as different size classes.



## Result

Tested with Ryzen 9 7950X.

OS: Ubuntu 18.04, VMWare

```
Using default tracefiles in ./traces/
Measuring performance with gettimeofday().

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.002187  2604
 1       yes   97%    5848  0.002074  2820
 2       yes   98%    6648  0.002707  2456
 3       yes   99%    5380  0.002461  2186
 4       yes   99%   14400  0.002626  5483
 5       yes   92%    4800  0.002500  1920
 6       yes   91%    4800  0.002438  1969
 7       yes   95%   12000  0.004557  2633
 8       yes   88%   24000  0.009442  2542
 9       yes   29%   14401  0.026028   553
10       yes   49%   14401  0.004076  3533
Total          85%  112372  0.061095  1839

Perf index = 51 (util) + 40 (thru) = 91/100
```

