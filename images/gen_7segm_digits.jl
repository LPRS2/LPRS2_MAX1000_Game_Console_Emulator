#!/usr/bin/env julia

function gen_digit(color_name, color, digit)
	file_name = "$(color_name)_$digit"
	open("$file_name.svg", "w") do of
		p(args...) = println(of, args...)
		p("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")
		p("<!DOCTYPE svg>")
		p("<svg version=\"1.1\" width=\"32\" height=\"64\">")
		p("<path fill=\"$color\" d=\"M5 3  L8 0  L8 6  Z\"/>")
		p("</svg>")
	end
	
	run(`inkscape -z -w 32 -h 64 -b black $file_name.svg -e $file_name.png`)
	run(`eog $file_name.png`)
	
	return file_name
end

function gen_digit_set(color_name, color)
	for d in 0:0
		gen_digit(color_name, color, d)
	end
end

gen_digit_set("green", "#00CC00")
