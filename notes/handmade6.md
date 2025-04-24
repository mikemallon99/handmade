User Input & Sound
- sometimes the user might not have xinput, but you wont want that to cause the game to not load
    - you can load windows things into your program yourself to fix this
    - grab function depfines yourself directly from the librarys .h file
- no semicolons after defines
- go name everything internal 

DirectSound
- performance warning when you have to go int -> bool. must do yourself
- bool32 to say "i intend this to be bool but i dont need compiler to do 0 or 1 forcing"
