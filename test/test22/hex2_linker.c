/* -*- c-file-style: "linux";indent-tabs-mode:t -*- */
/* Copyright (C) 2017 Jeremiah Orians
 * Copyright (C) 2017 Jan Nieuwenhuizen <janneke@gnu.org>
 * This file is part of MES
 *
 * MES is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with stage0.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


#define max_string 4096
//CONSTANT max_string 4096
#define TRUE 1
//CONSTANT TRUE 1
#define FALSE 0
//CONSTANT FALSE 0

void file_print(char* s, FILE* f);
int match(char* a, char* b);
char* numerate_number(int a);
int numerate_string(char *a);

struct input_files
{
	struct input_files* next;
	char* filename;
};

struct entry
{
	struct entry* next;
	unsigned target;
	char* name;
};

FILE* output;
struct entry* jump_table;
int BigEndian;
int Base_Address;
int Architecture;
int ByteMode;
int exec_enable;
int ip;
char* scratch;
char* scratch2;

int consume_token(FILE* source_file, char* s)
{
	int i = 0;
	int c = fgetc(source_file);
	do
	{
		if(NULL != s) s[i] = c;
		i = i + 1;
		c = fgetc(source_file);
	} while((' ' != c) && ('\t' != c) && ('\n' != c) && '>' != c);

	return c;
}

unsigned GetTarget(char* c)
{
	struct entry* i;
	for(i = jump_table; NULL != i; i = i->next)
	{
		if(match(c, i->name))
		{
			return i->target;
		}
	}
	file_print("Target label ", stderr);
	file_print(c, stderr);
	file_print(" is not valid\n", stderr);
	exit(EXIT_FAILURE);
}

int storeLabel(FILE* source_file, int ip)
{
	struct entry* entry = calloc(1, sizeof(struct entry));

	/* Prepend to list */
	entry->next = jump_table;
	jump_table = entry;

	/* Store string */
	entry->name = calloc(max_string + 1, sizeof(char));
	int c = consume_token(source_file, entry->name);

	/* Ensure we have target address */
	entry->target = ip;
	return c;
}

void range_check(int displacement, int number_of_bytes)
{
	if(4 == number_of_bytes) return;
	else if (3 == number_of_bytes)
	{
		if((8388607 < displacement) || (displacement < -8388608))
		{
			file_print("A displacement of ", stderr);
			file_print(numerate_number(displacement), stderr);
			file_print(" does not fit in 3 bytes\n", stderr);
			exit(EXIT_FAILURE);
		}
		return;
	}
	else if (2 == number_of_bytes)
	{
		if((32767 < displacement) || (displacement < -32768))
		{
			file_print("A displacement of ", stderr);
			file_print(numerate_number(displacement), stderr);
			file_print(" does not fit in 2 bytes\n", stderr);
			exit(EXIT_FAILURE);
		}
		return;
	}
	else if (1 == number_of_bytes)
	{
		if((127 < displacement) || (displacement < -128))
		{
			file_print("A displacement of ", stderr);
			file_print(numerate_number(displacement), stderr);
			file_print(" does not fit in 1 byte\n", stderr);
			exit(EXIT_FAILURE);
		}
		return;
	}

	file_print("Invalid number of bytes given\n", stderr);
	exit(EXIT_FAILURE);
}

void outputPointer(int displacement, int number_of_bytes)
{
	unsigned value = displacement;

	/* HALT HARD if we are going to do something BAD*/
	range_check(displacement, number_of_bytes);

	if(BigEndian)
	{ /* Deal with BigEndian */
		if(4 == number_of_bytes) fputc((value >> 24), output);
		if(3 <= number_of_bytes) fputc(((value >> 16)%256), output);
		if(2 <= number_of_bytes) fputc(((value >> 8)%256), output);
		if(1 <= number_of_bytes) fputc((value % 256), output);
	}
	else
	{ /* Deal with LittleEndian */
		while(number_of_bytes > 0)
		{
			unsigned byte = value % 256;
			value = value / 256;
			fputc(byte, output);
			number_of_bytes = number_of_bytes - 1;
		}
	}
}

int Architectural_displacement(int target, int base)
{
	if(0 == Architecture) return (target - base);
	else if(1 == Architecture) return (target - base);
	else if(2 == Architecture) return (target - base);

	file_print("Unknown Architecture, aborting before harm is done\n", stderr);
	exit(EXIT_FAILURE);
}

int ConsumePointer(char ch, FILE* source_file, char* s)
{
	/* Calculate pointer size*/
	if((37 == ch) || (38 == ch)) ip = ip + 4; /* Deal with % and & */
	else if((64 == ch) || (36 == ch)) ip = ip + 2; /* Deal with @ and $ */
	else if(33 == ch) ip = ip + 1; /* Deal with ! */
	else
	{
		file_print("storePointer given unknown\n", stderr);
		exit(EXIT_FAILURE);
	}

	return consume_token(source_file, s);
}

void storePointer(char ch, FILE* source_file)
{
	/* Get string of pointer */
	memset (scratch, 0, max_string + 1);
	int base_sep_p = (ConsumePointer(ch, source_file, scratch) == 62); /* '>' */

	/* Lookup token */
	int target = GetTarget(scratch);
	int displacement;

	int base = ip;
	/* Change relative base address to :<base> */
	if (base_sep_p)
	{
		memset (scratch2, 0, max_string + 1);
		consume_token (source_file, scratch2);
		base = GetTarget (scratch2);
	}

	displacement = Architectural_displacement(target, base);

	/* output calculated difference */
	if(33 == ch) outputPointer(displacement, 1); /* Deal with ! */
	else if(36 == ch) outputPointer(target, 2); /* Deal with $ */
	else if(64 == ch) outputPointer(displacement, 2); /* Deal with @ */
	else if(38 == ch) outputPointer(target, 4); /* Deal with & */
	else if(37 == ch) outputPointer(displacement, 4);  /* Deal with % */
	else
	{
		file_print("storePointer reached impossible case: ch=", stderr);
		fputc(ch, stderr);
		file_print("\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void line_Comment(FILE* source_file)
{
	int c = fgetc(source_file);
	while((10 != c) && (13 != c))
	{
		c = fgetc(source_file);
	}
}

int hex(int c, FILE* source_file)
{
	if (c >= '0' && c <= '9') return (c - 48);
	else if (c >= 'a' && c <= 'z') return (c - 87);
	else if (c >= 'A' && c <= 'Z') return (c - 55);
	else if (c == '#' || c == ';') line_Comment(source_file);
	return -1;
}

int octal(int c, FILE* source_file)
{
	if (c >= '0' && c <= '7') return (c - 48);
	else if (c == '#' || c == ';') line_Comment(source_file);
	return -1;
}

int binary(int c, FILE* source_file)
{
	if (c == '0' || c == '1') return (c - 48);
	else if (c == '#' || c == ';') line_Comment(source_file);
	return -1;
}


int hold;
int toggle;
void process_byte(char c, FILE* source_file, int write)
{
	if(16 == ByteMode)
	{
		if(0 <= hex(c, source_file))
		{
			if(toggle)
			{
				if(write) fputc(((hold * 16)) + hex(c, source_file), output);
				ip = ip + 1;
				hold = 0;
			}
			else
			{
				hold = hex(c, source_file);
			}
			toggle = !toggle;
		}
	}
	else if(8 ==ByteMode)
	{
		if(0 <= octal(c, source_file))
		{
			if(2 == toggle)
			{
				if(write) fputc(((hold * 8)) + octal(c, source_file), output);
				ip = ip + 1;
				hold = 0;
				toggle = 0;
			}
			else if(1 == toggle)
			{
				hold = ((hold * 8) + octal(c, source_file));
				toggle = 2;
			}
			else
			{
				hold = octal(c, source_file);
				toggle = 1;
			}
		}
	}
	else if(2 == ByteMode)
	{
		if(0 <= binary(c, source_file))
		{
			if(7 == toggle)
			{
				if(write) fputc((hold * 2) + binary(c, source_file), output);
				ip = ip + 1;
				hold = 0;
				toggle = 0;
			}
			else
			{
				hold = ((hold * 2) + binary(c, source_file));
				toggle = toggle + 1;
			}
		}
	}
}

void first_pass(struct input_files* input)
{
	if(NULL == input) return;
	first_pass(input->next);
	FILE* source_file = fopen(input->filename, "r");

	if(NULL == source_file)
	{
		file_print("The file: ", stderr);
		file_print(input->filename, stderr);
		file_print(" can not be opened!\n", stderr);
		exit(EXIT_FAILURE);
	}

	toggle = FALSE;
	int c;
	for(c = fgetc(source_file); EOF != c; c = fgetc(source_file))
	{
		/* Check for and deal with label */
		if(58 == c)
		{
			c = storeLabel(source_file, ip);
		}

		/* check for and deal with relative/absolute pointers to labels */
		if((33 == c) || (64 == c) || (36 == c) || (37 == c) || (38 == c))
		{ /* deal with 1byte pointer !; 2byte pointers (@ and $); 4byte pointers (% and &) */
			c = ConsumePointer(c, source_file, NULL);
			if (62 == c)
			{ /* deal with label>base */
				c = consume_token (source_file, NULL);
			}
		}
		else process_byte(c, source_file, FALSE);
	}
	fclose(source_file);
}

void second_pass(struct input_files* input)
{
	if(NULL == input) return;
	second_pass(input->next);
	FILE* source_file = fopen(input->filename, "r");

	/* Something that should never happen */
	if(NULL == source_file)
	{
		file_print("The file: ", stderr);
		file_print(input->filename, stderr);
		file_print(" can not be opened!\nWTF-pass2\n", stderr);
		exit(EXIT_FAILURE);
	}

	toggle = FALSE;
	hold = 0;

	int c;
	for(c = fgetc(source_file); EOF != c; c = fgetc(source_file))
	{
		if(58 == c) c = consume_token(source_file, NULL); /* Deal with : */
		else if((33 == c) || (64 == c) || (36 == c) || (37 == c) || (38 == c)) storePointer(c, source_file);  /* Deal with !, @, $, %  and & */
		else process_byte(c, source_file, TRUE);
	}

	fclose(source_file);
}

/* Standard C main program */
int main(int argc, char **argv)
{
	BigEndian = TRUE;
	jump_table = NULL;
	Architecture = 0;
	Base_Address = 0;
	struct input_files* input = NULL;
	output = stdout;
	char* output_file = "";
	exec_enable = FALSE;
	ByteMode = 16;
	scratch = calloc(max_string + 1, sizeof(char));
	scratch2 = calloc(max_string + 1, sizeof(char));

	int option_index = 1;
	while(option_index <= argc)
	{
		if(NULL == argv[option_index])
		{
			option_index = option_index + 1;
		}
		else if(match(argv[option_index], "--BigEndian"))
		{
			BigEndian = TRUE;
			option_index = option_index + 1;
		}
		else if(match(argv[option_index], "--LittleEndian"))
		{
			BigEndian = FALSE;
			option_index = option_index + 1;
		}
		else if(match(argv[option_index], "--exec_enable"))
		{
			exec_enable = TRUE;
			option_index = option_index + 1;
		}
		else if(match(argv[option_index], "-A") || match(argv[option_index], "--Architecture"))
		{
			Architecture = numerate_string(argv[option_index + 1]);
			option_index = option_index + 2;
		}
		else if(match(argv[option_index], "-b") || match(argv[option_index], "--binary"))
		{
			ByteMode = 2;
			option_index = option_index + 1;
		}
		else if(match(argv[option_index], "-B") || match(argv[option_index], "--BaseAddress"))
		{
			Base_Address = numerate_string(argv[option_index + 1]);
			option_index = option_index + 2;
		}
		else if(match(argv[option_index], "-h") || match(argv[option_index], "--help"))
		{
			file_print("Usage: ", stderr);
			file_print(argv[0], stderr);
			file_print(" -f FILENAME1 {-f FILENAME2} (--BigEndian|--LittleEndian)", stderr);
			file_print(" [--BaseAddress 12345] [--Architecture 12345]\nArchitecture", stderr);
			file_print(" 0: Knight; 1: x86; 2: AMD64\nTo leverage octal or binary", stderr);
			file_print(" input: --octal, --binary\n", stderr);
			exit(EXIT_SUCCESS);
		}
		else if(match(argv[option_index], "-f") || match(argv[option_index], "--file"))
		{
			struct input_files* temp = calloc(1, sizeof(struct input_files));
			temp->filename = argv[option_index + 1];
			temp->next = input;
			input = temp;
			option_index = option_index + 2;
		}
		else if(match(argv[option_index], "-o") || match(argv[option_index], "--output"))
		{
			output_file = argv[option_index + 1];
			output = fopen(output_file, "w");

			if(NULL == output)
			{
				file_print("The file: ", stderr);
				file_print(argv[option_index + 1], stderr);
				file_print(" can not be opened!\n", stderr);
				exit(EXIT_FAILURE);
			}
			option_index = option_index + 2;
		}
		else if(match(argv[option_index], "-O") || match(argv[option_index], "--octal"))
		{
			ByteMode = 8;
			option_index = option_index + 1;
		}
		else if(match(argv[option_index], "-V") || match(argv[option_index], "--version"))
		{
			file_print("hex2 0.3\n", stdout);
			exit(EXIT_SUCCESS);
		}
		else
		{
			file_print("Unknown option\n", stderr);
			exit(EXIT_FAILURE);
		}
	}

	/* Make sure we have a program tape to run */
	if (NULL == input)
	{
		return EXIT_FAILURE;
	}

	/* Get all of the labels */
	ip = Base_Address;
	first_pass(input);

	/* Fix all the references*/
	ip = Base_Address;
	second_pass(input);

	/* Set file as executable */
	if(exec_enable)
	{
		/* 488 = 750 in octal */
		if(0 != chmod(output_file, 488))
		{
			file_print("Unable to change permissions\n", stderr);
			exit(EXIT_FAILURE);
		}
	}

	return EXIT_SUCCESS;
}