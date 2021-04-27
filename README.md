# brightness

## Description

This small tool listens to file "actual_brightness" modification and reports current brightness percentage. Written in C. It can be used with i3blocks persistent block.

## How to build

`make release`

The result executable will be placed into bin directory.

## Usage

```
brightness 0.1 - read actual brightness value in non-blocking style.

Usage: brightness [options]
          
          Options:
          -a, --actual_brightness_path  	path to file with actual brightness string
          -m, --max_brightness_path     	path to file with max brightness string
          -h, --help                    	print this help.
          -V, --version                 	print version and exit.

```
