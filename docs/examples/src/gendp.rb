#!/usr/bin/env ruby
#
# Copyright(C) 2007 Simon Howard
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
# 
# Converts tunes into Doom PC speaker sound effects.
#

def convert_tune(tune)
	octave = 0

	segs = tune.downcase.split(/\//)

	result = []

	for seg in segs
		next if seg =~ /^\s*$/

		if seg =~ /\</
			octave -= 1
		elsif seg =~ /\>/
			octave += 1
		end

		if seg !~ /([abcdefgr][\-\+]?)/
			raise "Unknown note #{seg}"
		end

		note = $1

		pitch = 0

		if note == "r"
			# rest
		else
			scale = {
				"a-" => 31,
				"a"  => 33,
				"a+" => 35,
				"b-" => 35,
				"b"  => 37,
				"c"  => 39,
				"c+" => 41,
				"d-" => 41,
				"d"  => 43,
				"d+" => 45,
				"e-" => 45,
				"e"  => 47,
				"f"  => 49,
				"f+" => 51,
				"g-" => 51,
				"g"  => 53,
				"g+" => 55,
			}

			pitch = scale[note]

			if pitch == nil
				raise "Unknown note #{note}"
			end
		end

		pitch += octave * 24

		if seg !~ /(\d+)/
			raise "No duration given"
		end

		duration = $1.to_i

		actual_dur = 
			if seg =~ /\./
				(90 * 3) / duration
			else
				(90 * 2) / duration
			end

		actual_dur.times do 
			result.push(pitch)
		end
	end

	putc(0)
	putc(0)
	putc(result.length & 0xff)
	putc((result.length >> 8) & 0xff)

	for i in result
		putc(i)
	end
end

# Read tune from standard input

tune = ""

STDIN.each_line do |s|
	s = s.chomp
	if s =~ /^\;/
		# comment
		next
	end
	tune += s
end

# Convert and print to standard output

convert_tune(tune)

