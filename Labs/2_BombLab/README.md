# Procedure of Solving Bomb lab

## Phase 1

First we set two breakpoints, one is at line 73, the other is the entry of the function `phase_1`

```c
73    input = read_line();             /* Get input                   */
74    phase_1(input);                  /* Run the phase               */
```

<img src="./imgs/1_set_brpts.png" alt="1_set_brpts" style="zoom:50%;" />

Second, create a new file answer.txt as input strings for bomb phases and pass it through the bomb exec-file. I use below strings as input:

```
1
22
333
4444
55555
666666
```

then `run answer.txt` and it hits the breakpoint 1:

![image-20230811134644560](.\imgs\1_input_param_phase_1.png)

From the assembly code we can see what happen here:

1. call read_line() func and store the return value in `%rax`
2. copy the `%rax` to `%rdi` to pass the parameter for function `phase_1`

Use `nexti` to finish the `read_line` func call.

We can inspect the value of `%rax` and it is exactly the first line of answer.txt:

```shell
(gdb) x/s $rax
0x603780 <input_strings>:       "1"
```

Then use `stepi` to enter the `phase_1`, below shows the entry code of it:

![image-20230811135355728](.\imgs\1_phase1_entry.png)

It pass something to `%esi` and we don't know what it is yet. But we can see then it calls `string_not_equal` func, let's step into it and see what happens:

![image-20230811135640462](D:\MyCareerFileStroage\UniversityCourses\CMU 15-213 CSAPP\Labs\2_BombLab\imgs\1_entry_strings_not_equal.png)

From the function name we know that it's calling `string_length` to calculate the length of two strings and then compare these lengths. 

At the beginning, `%rdi` is our string "1" in answer.txt. Both `%rdi` and `%rsi`  are parameters passed to the func strings_not_equal. So here we can now that the addr of "password" is in `%rsi`, which is 0x402400. Use command below and we can get the password:

```shell
(gdb) x/s 0x402400
0x402400:       "Border relations with Canada have never been better."
```

It is easy to check the answer since the length of the sentence is same as the return value of second `string_length` func call, which is 52. Or you can check it by set another breakpoint after the phase_1 in `main` and you will see the "Phase 1 defused. How about the next one?" printed.



## Phase 2

找到1个`scanf`函数，根据其前面一个`mov`指令应该可以确定这个指令的内容是`scanf`函数的第一个形参format，对其用`x /s`查看可知道是`"%d %d %d %d %d %d"`，所以这一次一行需要输入6个数字，之后的比较容易看出，取第一个数为a，然后每次a=a+a，再将这个数与下一个数做比较，因此这一行的输入要求是1起始q=2的6个数的等比数列。

```shell
1 2 4 8 16 32
```



## Phase 3

与Phase 2类似容易知道要求输入2个数字
对于输入的第1个数字，根据如下无符号大于比较可知，输入的数据范围是[0, 7]，否则就炸弹爆炸。

```assembly
compl $0x7, 0x8(%rsp)
ja 0x400fad
```

同时该数字会用于间接跳转`jmpq`指令，具体是`jmpq addr(, %rax, 8)`，其中`%rax`的值就是我们输入的第1个数字，取出[addr+8*%rax]地址的内容，跳转到这个内容指向的地址，可以通过如下命令查看该addr附近的内容：

```shell
x/8gx addr
```

最终根据跳转地址可以知道，是在做一些数字比较，因此只需要保证跳转的位置和要求的数字一样就行，具体有如下可选的8个输入：

```shell
0 207
1 311
2 707
3 256
4 389
5 206
6 682
7 327
```



## Phase 4

与Phase 2类似容易知道要求输入2个数字

对第二个数字，硬性要求是0；对第一个数字，要求其范围在[0, 14]之间，然后传给`int fun4(int val, int min, int max)`函数进行处理，其中min=0，max=14，并且要求fun4的返回值==0，因此关键就是弄清楚fun4()做了什么。

翻译后代码如下：

```c
int fun4(int val, int min, int max)
{
    /*
    int res = max;
    res -= min;	// res = max - min
    int tmp = res;
    tmp >>= 31;	// tmp = 0
    res += tmp;
    res >>= 1;	// res = (max - min) / 2
    tmp = res + min;	// tmp = (min + max) / 2, 防溢出
    汇编翻译后的源代码如上，似乎等价于下面的2行
    */
    int res;
    int mid = min + (max - min) / 2;
    
    if (mid > val)
    {
        max = mid - 1;
        res = 2 * fun4(val, min, max);
    }
    else
    {
        res = 0;
        if (mid < val)
        {
            min = mid + 1;
            res = 2 * fun4(edi, esi, edx) + 1;
        }
    }
    
    return res;
}
```

可知fun4是在做递归二分查找，而为了使最终递归返回值为0，那么`mid < val`这个语句就不能被满足。即递归过程中，应当只允许`mid>=val`，在最后一层递归时`mid == val`，其它层均`mid > val`。因此，输入的第一个数字必须<=7。之后只需要对[0, 7]一个个穷举就能得出所有符合条件的解如下：

```shell
7 0
3 0
1 0
0 0
```



## Phase 5

输入一个长度为6的字符串，然后在循环中对其每个字符进行处理后与"flyers"字符串进行比较，只有当处理后的结果=="flyers"时才算成功。核心代码翻译后大致如下：

```c
char* sentence = "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?";		// strlen(sentence) = 71

void phase_5(char* input)
{
    if (strlen(input) != 6)
        explode_bomb();
    
    char[6] pickup;
    for (int i = 0; i < 6; i++)
    {
		char c = input[i];
        char j = c & 0xf;
        pickup[i] = sentence[(int)j];
    }
    if (strings_not_equal(pickup, "flyers"))
        explode_bomb();
}

int strings_not_equal(char* a, char* b);	// return 1 if equals, else return 0
```

容易知道，要去使用输入字符对应的ascii码的低4位(0~15)作为索引，然后从`sentence`字符串中取出单个字符替换原字符，使得最终结果为"flyers"。

因此可选输入为：

```shell
f: 9 	--> ) 9 Y i y
l: 15	--> / O o
y: 14	--> . > N ^ n ~
e: 5	--> % 5 E U e u
r: 6	--> & F V f v
s: 7	--> ' 7 G W g w
```



## Phase 6

要求输入6个整数，每个数的取值范围是[1, 6]，之后会在一个二层循环内做判断，目的是要求这6个数两两互不相等（唯一，不重复）。之后对于每个输入的数`input[i]`，令`newInput[i] = 7 - input[i];`。

然后在内存`0x6032d0`处存在这样一个Node数组（连续的Node节点），同时这些Node也组成一个链表。

![6_NodeArrayBegin](.\imgs\6_NodeArrayBegin.png)

之后存在这样一个循环，循环每次将`targetNode`链表从起点开始前进`newInput[i]-1`步，将得到的链表节点的地址赋值给`targetAddr[i]`。假设循环完成后，`targetAddr`数组中链表地址代表的节点是node3, node4, node5, node1, node 2, node 6，然后下一个循环就是将原本的链表修改为3->4->5->1->2->6的顺序。修改完成后检查新链表的`val`值，要求其是降序排列。翻译成C语言代码如下：

```c
struct Node
{
    int val;
    int index;
    struct Node* next;
}

Node targetNode[6] = {{0x14c, 1, 0x6032e0},	// node1
                      {0x0a8, 2, 0x6032f0},	// node2
                      {0x39c, 3, 0x603300},	// node3
                      {0x2b3, 4, 0x603310},	// node4
                      {0x1dd, 5, 0x603320},	// node5
                      {0x1bb, 6, 0x000000},	// node6
                     };	// &targetNode[0] == 0x6032d0

Node* targetAddr[6];
for (int i = 0; i < 6; i++)
{
    int val = newInput[i];
    Node* node = &targetNode;
    for (int j = 1; j < val; j++)
    {
        node = node->next;
    }
    targetAddr[i] = node;
}

for (int i = 0; i < 5; i++)
{
    targetAddr[i]->next = targetAddr[i + 1];
}
targetAddr[5]->next = 0;

for (int i = 5; i > 0; i--)
{
    rax = addr->next;
    if (addr->val < rax->val)	// 要求新链表的val降序排列
        explode_bomb();
    addr++;
}
```

由上述分析可知，前进`newInput[i] - 1`步实际上就决定了新链表的顺序。

由此可得最终结果如下：

```c
newInput - 1 			--> 2 3 4 5 0 1
newInput 				--> 3 4 5 6 1 2
input = 7 - newInput 	--> 4 3 2 1 6 5    
```

