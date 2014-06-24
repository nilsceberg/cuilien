#include <stdio.h>

#include "vm.h"
#include "args.h"
#include "mem.h"
#include "error.h"
#include "cpu.h"
#include "vector.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void print_vector_info(vector_t* vector)
{
	printf("Element size: %d | Size: %d | Capacity: %d\n", vector->element_size, vector->size, vector->capacity);
	int i;
	printf("Listing:\n");
	for(i=0; i<vector->size; ++i)
	{
		char* string;
		vector_get(vector, i, &string);
		printf("* %s\n", string);
	}
	printf("---\n");
}

int main(int argc, char** args)
{
	struct args_info arguments;
	args_parse(args, argc, &arguments);

	printf("Program: %s\n", arguments.program);

	int error;

	memory_t* memory = mem_init(4096);
	if(error_last)
	{
		error_print(error_last);
		return 1;
	}

	cpu_handle cpu = cpu_init(memory);
	if(error_last)
	{
		error_print(error_last);
		return 1;
	}

	if(error = vm_init(&arguments))
		error_print(error);

	//mem_read_short(mem_resolve(memory, 0x104));

	page_info_t shared_info = { PAGE_WRITE, false };
	c_byte* shared_mem = malloc(sizeof(c_byte) * C_PAGE_SIZE);
	//page_map(&memory->page_table, 0, &shared_info, shared_mem);

	// Load test program into memory
	c_addr programStart = 0xff000000;

	int file = open(arguments.program, O_RDONLY);

	if(file < 0)
	{
		printf("File not found.\n");
		return 1;
	}

	c_byte buffer[256];
	ssize_t chunk_length;
	size_t bytes_written = 0;
	while((chunk_length = read(file, buffer, 256)) > 0)
	{
		int i;
		for(i=0; i<chunk_length; ++i)
		{
			mem_write_byte(memory, programStart + bytes_written, buffer[i]);
			++bytes_written;
		}
	}

	close(file);

	
	// Write some fun data to play with
	mem_write_long(memory, 0x666, 1337);
	mem_write_long(memory, 1337, 0xdeadbeef);
	
	// Initialize registers to identifiable values
	cpu->reg.a = 1000;
	cpu->reg.b = 2000;
	cpu->reg.c = 3000;
	cpu->reg.d = 4000;

	// Move instruction pointer to start of program and step through the 7 instructions
	cpu->reg.ip = programStart;
	int i;
	for(i = 0; i<20; ++i)
	{
		cpu_step(cpu);
		printf("--\n");
	}
	
	free(shared_mem);

	cpu_free(cpu);
	mem_free(memory);

	return 0;
}
