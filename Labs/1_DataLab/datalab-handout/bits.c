/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  return ~(~(~x & y) & ~(x & ~y));
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1 << 31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  int a = x + 1;
  int b = ~a;
  return !(x ^ b) & !!a;
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  int mask = 0xaa;
  mask <<= 8, mask |= 0xaa;
  mask <<= 8, mask |= 0xaa;
  mask <<= 8, mask |= 0xaa;
  return !((x & mask) ^ mask);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x + 1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 * 
 *  Solution: first check the high 24 bits == 0x3, then check the low 4 bits > 9
 *            assume b3b2b1b0 as the low 4 bits, then when b3 & (b2|b1) the num > 9
 */
int isAsciiDigit(int x) {
  int mask = (1 << 31) >> 27;   // 0xfffffff0
  int highBitsNotEqual3 = (x & mask) ^ 0x30;
  int b3b1 = (x & 0xa) ^ 0xa;
  int b3b2 = (x & 0xc) ^ 0xc;
  return !(highBitsNotEqual3 | !b3b1 | !b3b2);
} 
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  int isZero = !x;
  int fullExt = ~isZero + 1;  // extend 0 and 1 to 0x00000000 and 0xffffffff
  return (y & ~fullExt) | (z & fullExt);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int signBitMask = 1 << 31;
  int xMinusY = x + (~y + 1);
  int resultSignBit = xMinusY & signBitMask;
  int xSignBit = x & signBitMask;
  int isOverflow = ((x ^ y) & signBitMask) & (resultSignBit ^ xSignBit);
  return !xMinusY | !!(~isOverflow & resultSignBit) | !!(isOverflow & xSignBit);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  int xSignBit = (x >> 31) & 1;
  int negX = ~x + 1;
  int negXSignBit = (negX >> 31) & 1;
  int isNotZero = xSignBit | negXSignBit;
  return isNotZero ^ 1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5     --- 0 1100
 *            howManyBits(298) = 10   --- 0 1 0010 1010
 *            howManyBits(-5) = 4     --- 1 011
 *            howManyBits(0)  = 1     --- 0
 *            howManyBits(-1) = 1     --- 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 * 
 *  Solution: for pos, find the no. of most left 1, res = no. + 1
 *            for neg, find the no. of most left 0, res = no. + 1
 *            so for neg we can ~x and find the most left 1 like pos.
 *            no. counts from 1 but can be zero if unsatisfied (e.g. howManyBits(0) and howManyBits(-1))
 *            further thinking, that is to find the times of right shift operations to make x become 0
 */
int howManyBits(int x) {
  int rs31 = x >> 31;
  int isRS16not0, isRS8not0, isRS4not0, isRS2not0, isRS1Not0;
  int needRS16, needRS8, needRS4, needRS2, needRS1;
  int rsCount;
  x = (x & ~rs31) | (~x & rs31);

  isRS16not0 = !!(x >> 16);
  needRS16 = isRS16not0 << 4;
  x >>= needRS16;

  isRS8not0 = !!(x >> 8);
  needRS8 = isRS8not0 << 3;
  x >>= needRS8;

  isRS4not0 = !!(x >> 4);
  needRS4 = isRS4not0 << 2;
  x >>= needRS4;

  isRS2not0 = !!(x >> 2);
  needRS2 = isRS2not0 << 1;
  x >>= needRS2;

  isRS1Not0 = !!(x >> 1);
  needRS1 = isRS1Not0;
  x >>= needRS1;

  rsCount = needRS16 + needRS8 + needRS4 + needRS2 + needRS1 + x;

  return rsCount + 1;
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  unsigned exp = (uf >> 23) & 0xff;
  unsigned frac = uf & 0x7fffff;
  unsigned result;

  if (exp == 0xff)   // NaN or inf
    return uf;

  if (exp == 0x00){
    frac <<= 1;
  }
  else{
    exp += 1;
  }

  result = (uf & 0x80000000) | (exp << 23) | frac;
  return result;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  unsigned signBit = uf >> 31;
  unsigned exp = (uf >> 23) & 0xff;
  unsigned frac = uf & 0x7fffff;
  int e, result;

  if (exp == 0xff)  // NaN or inf
    return 0x80000000u;

  if (exp == 0)     // denormalized
    return 0;
  
  e = exp - 127;
  if (e < 0)
    return 0;
  if (e > 31)
    return 0x80000000u;

  result = 1 << e;
  if (e < 23)
    result |= frac >> (23 - e);
  else
    result |= frac << (e - 23);
  if (signBit == 1)
    result = ~result + 1;

  return result;
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
  if (x > 127)
    return 0xff << 23;    // +INF

  if (x < -148)   // -23-126=-148
    return 0;

  if (x >= -148 && x <= -127) // denormalized, -126-1=-127
    return 1 << (x + 149);

  if (x > -127)
    return (x + 127) << 23;
  
  return 2;
}
