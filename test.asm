push 1
pop bx
start:
push bx
push bx
mul
out
call increment:
push bx
push 10
jb start:
hlt

increment:
push bx
push 1
add
pop bx
ret
