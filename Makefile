all : flash

TARGET:=m8
CH32V003FUN:=../ch32v003fun/ch32v003fun

include ../ch32v003fun/ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean

