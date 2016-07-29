; Replace CAL_DialogUpdate with this code

jnz ExecuteLoop

push 0
push 4
push ds
push TIMECOUNT_OFFSET
push 1 ; STDOUT
call CA_FARWRITE_FUNC_FAR_PTR
add sp, 0xA

push 0
push GAMESTATE_SIZE
push ds
push GAMESTATE_OFFSET
push 1
call CA_FARWRITE_FUNC_FAR_PTR
add sp, 0xA

push 0
push OBJLIST_SIZE
push ds
push OBJLIST_OFFSET
push 1
call CA_FARWRITE_FUNC_FAR_PTR
add sp, 0xA

jmp OBJ_PAST_LOOP_FAR_PTR ; Go past the initial PlayLoop objects loop

ExecuteLoop:

jmp OBJ_LOOP_FAR_PTR ; Go to the beginning of the initial PlayLoop objects loop
