push 0
pop bx
startb:
push 0
pop ax
starta:
push ax
push ax
mul
push bx
push bx
mul
add
push 25
jb draw:
drawreturn:
call incrementa:
push ax
push 10
jb starta:
call incrementb:
push bx
push 10
jb startb:
draw
hlt

draw:
push bx
push 10
mul
push ax
add
pop cx
push 1
pop [cx]
jmp drawreturn:

incrementa:
push ax
push 1
add
pop ax
ret
incrementb:
push bx
push 1
add
pop bx
ret
