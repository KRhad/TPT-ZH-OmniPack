// Android verison likes to try to compile this file
#ifndef ANDROID
#include <stdio.h>
#include <stdlib.h>
#include <bzlib.h>

void *file_load(char *fn, int *size)
{
	FILE *f = fopen(fn, "rb");
	void *s;

	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);
	s = malloc(*size);
	if (!s)
	{
		fclose(f);
		return NULL;
	}
	fread(s, *size, 1, f);
	fclose(f);
	return s;
}

int file_save(char *filename, void *file, int len)
{
	FILE *f = fopen(filename, "wb");
	if (!f)
		return 1;
	fwrite(file, len, 1, f);
	fclose(f);
	return 0;
}

int main(int argc, char *argv[])
{
	int exe_size, test_size, outputDataLen, dstate, ulen;
	unsigned char *outputData = NULL, *test_data = NULL, *decompress_data = NULL;
	void *exe_data = NULL;
	if (!argv[1] || !argv[2])
	{
		printf("Argument error\nIt takes the filename/filepath of the exe to compress as the first argument\nAnd the filename/filepath of the output file as the second\n");
		goto end;
	}
	exe_data = file_load(argv[1], &exe_size);
	if (!exe_data)
	{
		printf("Load Error\n");
		goto end;
	}

	outputDataLen = exe_size*2+12;
	outputData = (unsigned char*)malloc(outputDataLen);
	if (!outputData)
	{
		printf("Error, out of memory ...?\n");
		goto end;
	}

	outputData[0] = 0x42;
	outputData[1] = 0x75;
	outputData[2] = 0x54;
	outputData[3] = 0x54;
	outputData[4] = exe_size;
	outputData[5] = exe_size >> 8;
	outputData[6] = exe_size >> 16;
	outputData[7] = exe_size >> 24;

	if (BZ2_bzBuffToBuffCompress((char*)outputData+8, (unsigned*)(&outputDataLen), (char*)exe_data, exe_size, 9, 0, 0) != BZ_OK)
	{
		printf("Save Error\n");
		goto end;
	}
	
	printf("compressed data: %d to %d\n", exe_size, outputDataLen);

	if (file_save(argv[2], outputData, outputDataLen+12))
		printf("Error when writing file\n", outputDataLen);
	else
		printf("Successfully packaged file\n");
	
	
	//test to see if was compressed right
	test_data = file_load(argv[2], &test_size);
	if (!test_data || test_size < 16)
	{
		printf("Load Error\n");
		goto end;
	}
	ulen  = (unsigned char)test_data[4];
	ulen |= ((unsigned char)test_data[5])<<8;
	ulen |= ((unsigned char)test_data[6])<<16;
	ulen |= ((unsigned char)test_data[7])<<24;
	decompress_data = (char *)malloc(ulen);
	dstate = BZ2_bzBuffToBuffDecompress((char *)decompress_data, (unsigned *)&ulen, (char *)(test_data+8), test_size-8, 0, 0);
	if (dstate)
	{
		printf("Decompression failure: %d, %d, %d!\n", dstate, ulen, test_size);
		goto end;
	}

end:
	if (exe_data)
		free(exe_data);
	if (outputData)
		free(outputData);
	if (test_data)
		free(test_data);
	if (decompress_data)
		free(decompress_data);
	return 0;
}
#endif
