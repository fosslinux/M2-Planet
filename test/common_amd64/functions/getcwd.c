/* Copyright (C) 2020 Jeremiah Orians
 * This file is part of M2-Planet.
 *
 * M2-Planet is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * M2-Planet is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with M2-Planet.  If not, see <http://www.gnu.org/licenses/>.
 */

#include<stdlib.h>
//CONSTANT PATH_MAX 4096
#define PATH_MAX 4096

char* getcwd(char* buf, size_t size)
{
	asm("LOAD_EFFECTIVE_ADDRESS_rdi %16"
	"LOAD_INTEGER_rdi"
	"LOAD_EFFECTIVE_ADDRESS_rsi %8"
	"LOAD_INTEGER_rsi"
	"LOAD_IMMEDIATE_rax %79"
	"SYSCALL");
}

char* getwd(char* buf)
{
	return getcwd(buf, PATH_MAX);
}

char* get_current_dir_name()
{
	return getcwd(malloc(PATH_MAX), PATH_MAX);
}
