#!/usr/bin/env python
from __future__ import print_function
import sys
import numpy as np
import itertools

import csv

HAS_FLUID_LEFT = 2**0
HAS_FLUID_RIGHT = 2**1
HAS_FLUID_BOTTOM = 2**2
HAS_FLUID_TOP = 2**3
HAS_FLUID_FRONT = 2**4
HAS_FLUID_BACK = 2**5
IS_FLUID = 2**6
IS_OBSTACLE = 2**7
IS_BOUNDARY = 2**8

def reverse_enumerate(iterable):
    """
    Enumerate over an iterable in reverse order while retaining proper indexes
    """
    return itertools.izip(reversed(xrange(len(iterable))), reversed(iterable))


class grid:	
	def __init__(self, i_max, j_max, k_max):
		self.__data = [[[1 for i in range(i_max + 2)] for j in range(j_max + 2)] for k in range(k_max + 2)]
		self.__i_max = i_max
		self.__j_max = j_max
		self.__k_max = k_max

		for k in range(self.__k_max + 2):
			for j in range(self.__j_max + 2):
				for i in range(self.__i_max + 2):
					if i == 0 or j == 0 or k == 0 or i == self.__i_max + 1 or j == self.__j_max + 1 or k == self.__k_max + 1:
						self[i, j, k] = 0
		
	def __getitem__(self, tup):
		i, j, k = tup
		return self.__data[k][j][i]
  
	def __setitem__(self, tup, val):
		i, j, k = tup
		self.__data[k][j][i] = val
		
	def __str__(self):
		output = ""
		for k, plane in enumerate(self.__data):
			for j, row in enumerate(plane):
			  output += str(row)
			  if j != len(plane) - 1:
				output += "\n"
				
			output += "\n"
			output += "\n"
		
		return output

	def insert_rectangle(self, i_begin, i_end, j_begin, j_end, k_begin, k_end):
		for k in range(k_begin, k_end + 1):
			for j in range(j_begin, j_end + 1):
				for i in range(i_begin, i_end + 1):
					self[i, j, k] = 0
		
	def write_to(self, file_path):
		with open(file_path, 'wb') as csvfile:
			gridwriter = csv.writer(csvfile, delimiter=',', quotechar='"')

			for k, plane in reverse_enumerate(self.__data):
				for j, row in reverse_enumerate(plane):
					outrow = [0] * len(row)
										
					for i, cell in enumerate(row):
						outvalue = 0
						
						if cell == 1:
							outvalue = IS_FLUID
						elif i == 0 or i == self.__i_max + 1 or j == 0 or j == self.__j_max + 1 or k == 0 or k == self.__k_max + 1:
							outvalue = IS_BOUNDARY + IS_OBSTACLE
						else:
							outvalue = IS_OBSTACLE
						
						if i != 0 and self[i - 1, j, k] == 1:
							outvalue += HAS_FLUID_LEFT
						if i != self.__i_max + 1 and self[i + 1, j, k] == 1:
							outvalue += HAS_FLUID_RIGHT
						if j != 0 and self[i, j - 1, k] == 1:
							outvalue += HAS_FLUID_FRONT
						if j != self.__j_max + 1 and self[i, j + 1, k] == 1:
							outvalue += HAS_FLUID_BACK
						if k != 0 and self[i, j, k - 1] == 1:
							outvalue += HAS_FLUID_BOTTOM
						if k != self.__k_max + 1 and self[i, j, k + 1] == 1:
							outvalue += HAS_FLUID_TOP
							
						outrow[i] = outvalue				  
				  
					gridwriter.writerow(outrow)
		

if len(sys.argv) != 5:
	print("Usage:", sys.argv[0] , "i_max j_max k_max <output grid>", file=sys.stderr)
	sys.exit(0)

i_max = int(sys.argv[1])
j_max = int(sys.argv[2])
k_max = int(sys.argv[3])

outfile_path = sys.argv[4]

b = grid(i_max, j_max, k_max)
b.insert_rectangle(0, 1 + i_max/4, 0, j_max + 1, 0, 1 + k_max/2)

								
b.write_to(outfile_path)
				

				
