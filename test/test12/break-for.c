/* Copyright (C) 2016 Jeremiah Orians
 * This file is part of stage0.
 *
 * stage0 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * stage0 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with stage0.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Validate that call statements behave correctly */
int putchar(int);
void exit(int);

int main()
{
	int i = 65;
	for(i = 65; i <= 120; i = i + 1)
	{
		putchar(i);
		if(90 == i)
		{
			break;
		}
	}

	putchar(10);

	for(i = 65; i <= 120; i = i + 1)
	{
		int j = i;
		for(j = i; j <= 90; j = j + 1)
		{
			if(70 == i)
			{
				break;
			}
			putchar(j);
		}
		putchar(10);
		if(90 == i) break;
	}
	return 0;
}
