/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes (8 ints).
 * so there are 32 blocks.
 */ 
#include <stdio.h>
#include "cachelab.h"

#define min(a, b) (a > b ? b : a)

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	if (M == 32)
	{
		int bsize = 8;
		int en = bsize * (M / bsize);
		int a0, a1, a2, a3, a4, a5, a6, a7;
		for (int i = 0; i < en; i += bsize)
		{
			for (int j = 0; j < en; j += bsize)
			{
				for (int bi = i, bj = j; bi < i + bsize; bi++, bj++)
				{
					a0 = A[bi][j];
					a1 = A[bi][j+1];
					a2 = A[bi][j+2];
					a3 = A[bi][j+3];
					a4 = A[bi][j+4];
					a5 = A[bi][j+5];
					a6 = A[bi][j+6];
					a7 = A[bi][j+7];
					B[bj][i] = a0;
					B[bj][i+1] = a1;
					B[bj][i+2] = a2;
					B[bj][i+3] = a3;
					B[bj][i+4] = a4;
					B[bj][i+5] = a5;
					B[bj][i+6] = a6;
					B[bj][i+7] = a7;
				}

				for (int k = 0; k < bsize; k++)
				{
					for (int s = k + 1; s < bsize; s++)
					{
						a0 = B[j + k][i + s];
						B[j + k][i + s] = B[j + s][i + k];
						B[j + s][i + k] = a0;
					}
				}
			}
		}
	}
	else if (M == 64)
	{
		int bsize = 8;
		int a0, a1, a2, a3, a4, a5, a6, a7, tmp;
		for (int i = 0; i < N; i += bsize)
		{
			for (int j = 0; j < M; j += bsize)
			{
				for (int k = 0; k < bsize / 2; k++)
				{
					// A top left
					a0 = A[i + k][j];
					a1 = A[i + k][j + 1];
					a2 = A[i + k][j + 2];
					a3 = A[i + k][j + 3];
					// A top right
					a4 = A[i + k][j + 4];
					a5 = A[i + k][j + 5];
					a6 = A[i + k][j + 6];
					a7 = A[i + k][j + 7];

					// copy to B
					B[j + k][i] = a0;
					B[j + k][i + 1] = a1;
					B[j + k][i + 2] = a2;
					B[j + k][i + 3] = a3;
					B[j + k][i + 4] = a4;
					B[j + k][i + 5] = a5;
					B[j + k][i + 6] = a6;
					B[j + k][i + 7] = a7;
				}
				for (int k = 0; k < bsize / 2; k++)
				{
					for (int s = k + 1; s < bsize / 2; s++)
					{
						// top-left
						a0 = B[j + k][i + s];
						B[j + k][i + s] = B[j + s][i + k];
						B[j + s][i + k] = a0;
						// top-right
						a1 = B[j + k][i + s + 4];
						B[j + k][i + s + 4] = B[j + s][i + k + 4];
						B[j + s][i + k + 4] = a1;
					}
				}
				for (int k = 0; k < bsize / 2; k++)
				{
					// buf 1				// buf 2
					a0 = A[i + 4][j + k], 	a4 = A[i + 4][j + 4 + k];
					a1 = A[i + 5][j + k], 	a5 = A[i + 5][j + 4 + k];
					a2 = A[i + 6][j + k], 	a6 = A[i + 6][j + 4 + k];
					a3 = A[i + 7][j + k], 	a7 = A[i + 7][j + 4 + k];
					
					// B's top right transpose to bottom left (by exchanging)
					tmp = B[j + k][i + 4], B[j + k][i + 4] = a0, a0 = tmp;
					tmp = B[j + k][i + 5], B[j + k][i + 5] = a1, a1 = tmp;
					tmp = B[j + k][i + 6], B[j + k][i + 6] = a2, a2 = tmp;
					tmp = B[j + k][i + 7], B[j + k][i + 7] = a3, a3 = tmp;
					// write back to B's bottom left row and transpose B's bottom right
					B[j + k + 4][i + 0] = a0, B[j + k + 4][i + 4] = a4;
					B[j + k + 4][i + 1] = a1, B[j + k + 4][i + 5] = a5;
					B[j + k + 4][i + 2] = a2, B[j + k + 4][i + 6] = a6;
					B[j + k + 4][i + 3] = a3, B[j + k + 4][i + 7] = a7;
				}
			}
		}
	}
	else
	{
		int a0, a1, a2, a3, a4, a5, a6, a7;
		for (int i = 0; i < N; i += 8) 
		{
			for (int j = 0; j < M; j += 23) 
			{
				if (i + 8 <= N && j + 23 <= M) 
				{
					for (int s = j; s < j + 23; s++) 
					{
						a0 = A[i][s];
						a1 = A[i + 1][s];
						a2 = A[i + 2][s];
						a3 = A[i + 3][s];
						a4 = A[i + 4][s];
						a5 = A[i + 5][s];
						a6 = A[i + 6][s];
						a7 = A[i + 7][s];
						B[s][i + 0] = a0;
						B[s][i + 1] = a1;
						B[s][i + 2] = a2;
						B[s][i + 3] = a3;
						B[s][i + 4] = a4;
						B[s][i + 5] = a5;
						B[s][i + 6] = a6;
						B[s][i + 7] = a7;
					}
				} 
				else 
				{
					for (int k = i; k < min(i + 8, N); k++) 
					{
						for (int s = j; s < min(j + 23, M); s++) 
						{
							B[s][k] = A[k][s];
						}
					}
				}
			}
		}
	}
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, tmp;

	for (i = 0; i < N; i++) {
		for (j = 0; j < M; j++) {
			tmp = A[i][j];
			B[j][i] = tmp;
		}
	}    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
	/* Register your solution function */
	registerTransFunction(transpose_submit, transpose_submit_desc); 

	/* Register any additional transpose functions */
	registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
	int i, j;

	for (i = 0; i < N; i++) {
		for (j = 0; j < M; ++j) {
			if (A[i][j] != B[j][i]) {
				return 0;
			}
		}
	}
	return 1;
}

